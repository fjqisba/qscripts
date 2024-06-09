#pragma once
#include <pro.h>
#include "ida.h"

struct collect_extlangs : extlang_visitor_t
{
	extlangs_t* langs;

	virtual ssize_t idaapi visit_extlang(extlang_t* extlang) override
	{
		langs->push_back(extlang);
		return 0;
	}

	collect_extlangs(extlangs_t* langs, bool select)
	{
		langs->qclear();
		this->langs = langs;
		for_all_extlangs(*this, select);
	}
};

bool get_basename_and_ext(
	const char* path,
	char** basename,
	char** ext,
	qstring& wrk_buf);

bool get_file_modification_time(
	const char* filename,
	qtime64_t* mtime = nullptr);

bool get_file_modification_time(
	const qstring& filename,
	qtime64_t* mtime = nullptr);

void make_abs_path(qstring& path, const char* base_dir = nullptr, bool normalize = false);

void normalize_path_sep(qstring& path);
