#include <bits/stdc++.h>
using namespace std;

#define MAX_WEIGHT 4000000

class Transaction {
public:
    string tx_id;
    int fee;
    int weight;
    vector<string> parents;

    // constructors
    Transaction() {}

    Transaction(string _tx_id, int _fee, int _weight, vector<string> _parents) {
        this->tx_id = _tx_id;
        this->fee = _fee;
        this->weight = _weight;
        this->parents = _parents;
    }
};

// global variables
// outputBlock contains ordered list of transactions the miner will perform
// txInOutputBlock is an unordered_map which tracks which transactions are present in the output block
// tempTxInOutputBlock is an unordered_map which tracks the temporary transactions
// blockWeight contains the total weight of the block

vector<string> outputBlock;
unordered_map<string, int> txInOutputBlock, tempTxInOutputBlock;
unordered_map<string, Transaction> transactionsMap;
int blockWeight;

// helper functions
// comparator for deciding the ranking of the transactions on basis of the better fees/weight ratio
bool comp(pair<string, float> x, pair<string, float> y) {
    return x.second >= y.second;
}

// split the string using delimeter and stores the split string in the data
void parseUsingDelimeter(string str, vector<string> &data, string delimiter) {
    int start = 0, end = str.find(delimiter);
    while(end != -1) {
        data.push_back(str.substr(start, end - start));
        start = end + 1;
        end = str.find(delimiter, start);
    }
    data.push_back(str.substr(start));

    return;
}

// helper function to parse transaction from each csv line and returns the transaction
Transaction parseTransactionFromInputRow(string row) {
    Transaction newTx = Transaction();

    vector<string> tx_data;
    parseUsingDelimeter(row, tx_data, ",");

    newTx.tx_id = tx_data[0];
    newTx.fee = stoi(tx_data[1]);
    newTx.weight = stoi(tx_data[2]);
    parseUsingDelimeter(tx_data[3], newTx.parents, ";");

    return newTx;
}

// get Ratio function gets the pair with first element as total fees and second element as total weight which is required to perform a valid transaction
pair<int, int> getRatio(Transaction tx) {
    pair<int, int> ratio = make_pair(tx.fee, tx.weight);

    // for no parent transactions
    if(tx.parents.size() == 0) {
        return ratio;
    }

    // getting the ratio for all the parents using recusrive function calls and adding it to childs ratio
    for(string parent : tx.parents) { 
        pair<int, int> parentsRatio = getRatio(transactionsMap[parent]);
        ratio.first += parentsRatio.first;
        ratio.second += parentsRatio.second;
    }

    return ratio;
}

// getWeightToWriteInBlock function gets the total weight of all the transactions which are yet to be written in block using recursive calls
int getWeightToWriteInBlock(Transaction tx) {
    if(txInOutputBlock.find(tx.tx_id) != txInOutputBlock.end()) {
        return 0;
    }
    
    if(tempTxInOutputBlock.find(tx.tx_id) != tempTxInOutputBlock.end()) {
        return 0;
    }
    
    int tempWeight = 0;
    for(string parent : tx.parents) {
        tempWeight += getWeightToWriteInBlock(transactionsMap[parent]);
    }

    tempTxInOutputBlock[tx.tx_id] = 1;
    tempWeight += tx.weight;

    return tempWeight;
}

// writeToBlock function is only executed when we are sure that weight doesn't cross the MAX_WEIGHT
void writeToBlock(Transaction tx) {
    // checks if the transaction is already written in the block or not
    if(txInOutputBlock.find(tx.tx_id) != txInOutputBlock.end()) {
        return;
    }

    // visit each parent and write the transaction into block
    for(string parent : tx.parents) {
        writeToBlock(transactionsMap[parent]);
    }

    // write the transaction to block after the all parent transactions are done
    txInOutputBlock[tx.tx_id] = 1;
    outputBlock.push_back(tx.tx_id);

    return;
}


int main() {
    ifstream inputFile;
    ofstream outputFile;

    inputFile.open("./mempool.csv", ios::in);
    outputFile.open("./block.txt", ios::out);

    string row; // storing the line input read from csv file
    Transaction tempTx; // helper temporary transaction
    getline(inputFile, row); // neglecting he first row containing the column headings

    while (getline(inputFile, row)) {
        tempTx = parseTransactionFromInputRow(row);
        transactionsMap[tempTx.tx_id] = tempTx;
    }

    vector<pair<string, float>> transactionsRanking; // containing the order in which transactions can be performed

    // counting the ratio of fees/weight by getting the weight and fees required to write transaction to block i.e adding the weight and fees of all the parent
    for(pair<string, Transaction> txid_tx_pair : transactionsMap) {
        pair<int, int> ratio = getRatio(transactionsMap[txid_tx_pair.first]);
        transactionsRanking.push_back({txid_tx_pair.first, (float)(ratio.first) / (ratio.second)});
    }

    sort(transactionsRanking.begin(), transactionsRanking.end(), comp);

    // first checking if the weight doesn't exceed the limit by performing temporary operations and then writing it to the block
    blockWeight = 0;
    for(pair<string, float> txid_ratio_pair : transactionsRanking) {
        tempTx = transactionsMap[txid_ratio_pair.first];
        tempTxInOutputBlock.clear();
        int tempWeight = getWeightToWriteInBlock(tempTx);
        
        if(tempWeight + blockWeight > MAX_WEIGHT) {
            continue;
        }
        blockWeight += tempWeight;
        writeToBlock(tempTx);
    }

    // write the block to the file
    for(string tx_id : outputBlock) {
        if(tx_id != "") {
            outputFile << tx_id << endl;
        }
    }
}
