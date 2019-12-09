#include "Key.h"
#include "Database.h"
#include "Sheet.h"

ForeignKey::ForeignKey(Sheet* s, json j, Database* db) : Key(s, j["data"]) {
    p = db->sheet[j["sid"].get<int>()]->p_key;
    p->f.push_back(this);
}

json ForeignKey::toJson() {
    json j;
    j["data"] = Key::toJson();
    j["sid"] = p->sheet->sheet_id;
    return j;
}