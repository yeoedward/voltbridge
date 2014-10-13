#ifndef _HARDCODED_H__
#define _HARDCODED_H_

#include <iostream>
#include <fstream>
#include <string>

using namespace std;

string getCatalogString() {
    ifstream catalogFile;
    catalogFile.open(
        "/home/eyeo/postgresql-9.3.5/contrib/"
        "nstore/voltdb/nstore/jar/catalog.txt");
    if (!catalogFile) {
        cout << "Couldn't open file." << endl;
        return "";
    }
    string catalogString;
    string line;
    while (getline(catalogFile, line)) {
        // O(n^2) string concat. You didn't see this.
        catalogString += line + "\n";
    }
    return catalogString;
}

string getPlan() {
    return "{\"PLAN_NODES\":[{\"ID\":1,\"PLAN_NODE_TYPE\":\"SEND\",\"CHILDREN_IDS\":[2]},{\"ID\":2,\"PLAN_NODE_TYPE\":\"SEQSCAN\",\"INLINE_NODES\":[{\"ID\":0,\"PLAN_NODE_TYPE\":\"PROJECTION\",\"OUTPUT_SCHEMA\":[{\"COLUMN_NAME\":\"TOWN\",\"EXPRESSION\":{\"TYPE\":32,\"VALUE_TYPE\":9,\"VALUE_SIZE\":64,\"COLUMN_IDX\":0}},{\"COLUMN_NAME\":\"COUNTY\",\"EXPRESSION\":{\"TYPE\":32,\"VALUE_TYPE\":9,\"VALUE_SIZE\":64,\"COLUMN_IDX\":1}},{\"COLUMN_NAME\":\"STATE\",\"EXPRESSION\":{\"TYPE\":32,\"VALUE_TYPE\":9,\"VALUE_SIZE\":2,\"COLUMN_IDX\":2}}]}],\"TARGET_TABLE_NAME\":\"TOWNS\",\"TARGET_TABLE_ALIAS\":\"TOWNS\"}],\"EXECUTE_LIST\":[2,1]}";
}

#endif /* _HARDCODED_H_ */
