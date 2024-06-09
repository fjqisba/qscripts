/*
IDASDK extension library (c) Elias Bachaalany.

Hexrays utilities
*/
#pragma once

#include <algorithm>
#include <map>
#include <memory>

#include <hexrays.hpp>

#include "xkernwin.hpp"

//----------------------------------------------------------------------------------
inline update_state_ah_t hexrays_default_enable_for_vd_expr = FO_ACTION_UPDATE([],
    auto vu = get_widget_vdui(widget);
    return (vu == nullptr) ? AST_DISABLE_FOR_WIDGET
                            : vu->item.citype == VDI_EXPR ? AST_ENABLE : AST_DISABLE;
);

inline update_state_ah_t hexrays_default_enable_for_vd = FO_ACTION_UPDATE([],
    auto vu = get_widget_vdui(widget);
    return vu == nullptr ? AST_DISABLE_FOR_WIDGET : AST_ENABLE;
);

//----------------------------------------------------------------------------------
class hexrays_ctreeparent_visitor_t : public ctree_parentee_t
{
private:
    std::map<const citem_t*, const citem_t*> parent;
    std::map<const ea_t, const citem_t*> ea2item;

public:
    int idaapi visit_expr(cexpr_t* e) override
    {
        ea2item[e->ea] = parent[e] = parent_expr();
        return 0;
    }

    int idaapi visit_insn(cinsn_t* ins) override
    {
        parent[ins] = parent_insn();
        return 0;
    }

    const citem_t* parent_of(const citem_t* item)
    {
        return parent[item];
    }

    const citem_t* by_ea(ea_t ea) const
    {
        auto p = ea2item.find(ea);
        return p == std::end(ea2item) ? nullptr : p->second;
    }

    bool is_ancestor_of(const citem_t* parent, const citem_t* item)
    {
        while (item != nullptr)
        {
            item = parent_of(item);
            if (item == parent)
                return true;
        }
        return false;
    }
};

using hexrays_ctreeparent_visitor_ptr_t = std::unique_ptr<hexrays_ctreeparent_visitor_t>;

//----------------------------------------------------------------------------------
inline ea_t get_selection_range(
    TWidget* widget, 
    ea_t* end_ea = nullptr, 
    int widget_type = BWN_DISASM)
{
    ea_t ea1 = BADADDR, ea2 = BADADDR;
    do
    {
        if (widget == nullptr || (widget_type != -1 && get_widget_type(widget) != widget_type))
            break;

        if (!read_range_selection(widget, &ea1, &ea2))
        {
            ea1 = get_screen_ea();

            if (ea1 == BADADDR)
                break;

            ea2 = next_head(ea1, BADADDR);
            if (ea2 == BADADDR)
                ea2 = ea1 + 1;
        }
    } while (false);
    if (end_ea != nullptr)
        *end_ea = ea2;

    return ea1;
}

//----------------------------------------------------------------------------------
inline const cinsn_t* hexrays_get_stmt_insn(
    cfunc_t* cfunc, 
    const citem_t* ui_item, 
    hexrays_ctreeparent_visitor_ptr_t *ohelper = nullptr)
{
    auto func_body = &cfunc->body;

    const citem_t* item = ui_item;
    const citem_t* stmt_item;

    hexrays_ctreeparent_visitor_t* helper = nullptr;

    if (ohelper != nullptr)
    {
        // Start a new helper
        if (*ohelper == nullptr)
        {
            helper = new hexrays_ctreeparent_visitor_t();
            helper->apply_to(func_body, nullptr);

            ohelper->reset(helper);
        }
        else
        {
            helper = ohelper->get();
        }
    }

    auto get_parent = [func_body, &helper](const citem_t* item)
    {
        return helper == nullptr ? func_body->find_parent_of(item)
                                 : helper->parent_of(item);
    };

    // Get the top level statement from this item
    for (stmt_item = item; 
         item != nullptr && item->is_expr(); 
         item = get_parent(item))
    {
        stmt_item = item;
    }

    // ...then the actual instruction item
    if (stmt_item->is_expr())
        stmt_item = get_parent(stmt_item);

    return (const cinsn_t*)stmt_item;
}

//----------------------------------------------------------------------------------
inline bool hexrays_get_stmt_block_pos(
    cfunc_t* cfunc,
    const citem_t* stmt_item,
    cblock_t** p_cblock,
    cblock_t::iterator* p_pos,
    hexrays_ctreeparent_visitor_t* helper = nullptr)
{
    auto func_body = &cfunc->body;
    auto cblock_insn = (cinsn_t*)(
        helper == nullptr ? func_body->find_parent_of(stmt_item)
                          : helper->parent_of(stmt_item));

    if (cblock_insn == nullptr || cblock_insn->op != cit_block)
        return false;

    cblock_t* cblock = cblock_insn->cblock;

    for (auto p = cblock->begin(); p != cblock->end(); ++p)
    {
        if (&*p == stmt_item)
        {
            *p_pos = p;
            *p_cblock = cblock;
            return true;
        }
    }
    return false;
}

//----------------------------------------------------------------------------------
inline bool hexrays_are_acenstor_of(
    hexrays_ctreeparent_visitor_t* h, 
    cinsnptrvec_t& inst, 
    citem_t* item)
{
    for (auto parent : inst)
    {
        if (h->is_ancestor_of(parent, item))
            return true;
    }
    return false;
};

//----------------------------------------------------------------------------------
inline void hexrays_keep_lca_cinsns(
    cfunc_t* cfunc,
    hexrays_ctreeparent_visitor_t* helper,
    cinsnptrvec_t& bulk_list)
{
    cinsnptrvec_t new_list;
    while (!bulk_list.empty())
    {
        auto item = bulk_list.back();
        bulk_list.pop_back();

        if (!hexrays_are_acenstor_of(helper, bulk_list, item) && !hexrays_are_acenstor_of(helper, new_list, item))
            new_list.push_back(item);
    }
    new_list.swap(bulk_list);
}

//----------------------------------------------------------------------------------
inline void hexrays_find_expr(
    cfuncptr_t func,
    std::function<int(cexpr_t*)> cb,
    int flags = CV_FAST,
    citem_t* parent = nullptr)
{
    struct visitor_wrapper : public ctree_visitor_t
    {
        std::function<int(cexpr_t*)> cb;

        visitor_wrapper(int flags, std::function<int(cexpr_t*)> cb)
            : ctree_visitor_t(flags), cb(cb) {}

        virtual int idaapi visit_expr(cexpr_t* expr)
        {
            return cb(expr);
        }
    };

    visitor_wrapper v(flags, cb);
    v.apply_to(&func->body, parent);
}
