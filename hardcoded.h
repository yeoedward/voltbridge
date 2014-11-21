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
        "nstore/voltdb/nstore/catalog.txt");
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

string getSelectPlan() {
    return "{\"PLAN_NODES\":[{\"ID\":1,\"PLAN_NODE_TYPE\":\"SEND\",\"CHILDREN_IDS\":[2]},{\"ID\":2,\"PLAN_NODE_TYPE\":\"SEQSCAN\",\"INLINE_NODES\":[{\"ID\":0,\"PLAN_NODE_TYPE\":\"PROJECTION\",\"OUTPUT_SCHEMA\":[{\"COLUMN_NAME\":\"FIRST_NAME\",\"EXPRESSION\":{\"TYPE\":32,\"VALUE_TYPE\":5,\"COLUMN_IDX\":0}},{\"COLUMN_NAME\":\"SECOND_NAME\",\"EXPRESSION\":{\"TYPE\":32,\"VALUE_TYPE\":9,\"VALUE_SIZE\":30,\"COLUMN_IDX\":1}}]}],\"TARGET_TABLE_NAME\":\"PERSON\",\"TARGET_TABLE_ALIAS\":\"PERSON\"}],\"EXECUTE_LIST\":[2,1]}";
}

string getInsertPlan() {
    return "{\"PLAN_NODES\":[{\"ID\":4,\"PLAN_NODE_TYPE\":\"SEND\",\"CHILDREN_IDS\":[5]},{\"ID\":5,\"PLAN_NODE_TYPE\":\"INSERT\",\"CHILDREN_IDS\":[6],\"TARGET_TABLE_NAME\":\"PERSON\",\"MULTI_PARTITION\":false,\"FIELD_MAP\":[0,1]},{\"ID\":6,\"PLAN_NODE_TYPE\":\"MATERIALIZE\",\"OUTPUT_SCHEMA\":[{\"COLUMN_NAME\":\"FIRST_NAME\",\"EXPRESSION\":{\"TYPE\":31,\"VALUE_TYPE\":5,\"PARAM_IDX\":0}},{\"COLUMN_NAME\":\"SECOND_NAME\",\"EXPRESSION\":{\"TYPE\":31,\"VALUE_TYPE\":9,\"VALUE_SIZE\":30,\"PARAM_IDX\":1}}],\"BATCHED\":false}],\"EXECUTE_LIST\":[6,5,4]}";
}

string getPostInsertPlan() {
    return "{\"PLAN_NODES\":[{\"ID\":1,\"PLAN_NODE_TYPE\":\"SEND\",\"CHILDREN_IDS\":[2]},{\"ID\":2,\"PLAN_NODE_TYPE\":\"LIMIT\",\"CHILDREN_IDS\":[3],\"OFFSET\":0,\"LIMIT\":1,\"OFFSET_PARAM_IDX\":-1,\"LIMIT_PARAM_IDX\":-1,\"LIMIT_EXPRESSION\":null},{\"ID\":3,\"PLAN_NODE_TYPE\":\"RECEIVE\",\"OUTPUT_SCHEMA\":[{\"COLUMN_NAME\":\"modified_tuples\",\"EXPRESSION\":{\"TYPE\":32,\"VALUE_TYPE\":6,\"COLUMN_IDX\":0}}]}],\"EXECUTE_LIST\":[3,2,1]}";
}

#endif /* _HARDCODED_H_ */
