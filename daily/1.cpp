#include<stdio.h>
#include<iostream>
#include<fstream>
#include<sstream>
#include<string>
#include<algorithm>
#include<vector>
#include<chrono>
#include<ctime>
#include<cstring>
#include<iomanip>

#define ll long long

using namespace std;

void flag(string s){
    cout << s << endl;
}

void helper(string date){
    std::tm then{};

    std::istringstream in(date);

    if(!(in >> std::get_time(&then, "%d-%m-%Y")))
        std::cerr << "Conversion failed\n";
    
    mktime(&then);

    std::cout << std::put_time(&then, "%V\n");
}

struct Trade{
    ll entryVal, exitVal;
    string entryOHLCDataTime, exitOHLCDataTime;
};

struct OHLCData{
    string ticker, OHLCDataTime;
    double open, high, low, close;
    ll weekNo, dayNo;
};

int min(int a, int b){
    if(a < b) return a;
    return b;
}

bool operator<(OHLCData const& lhs, OHLCData const& rhs) noexcept{

    if(lhs.weekNo < rhs.weekNo){
        return true;
    }
    else if(lhs.weekNo == rhs.weekNo){
        return lhs.dayNo < rhs.dayNo;
    }
    else{
        return false;
    }
}

bool sorting_function(OHLCData& lhs, OHLCData& rhs){
    return lhs < rhs;
}

void print_OHLCData(OHLCData& t){
    cout << t.ticker << " " << t.OHLCDataTime << " " << t.weekNo << " " << t.dayNo << endl;
}

void addWeeknoDayno(vector<OHLCData>& dataList){
    string date;
    int day, week;

    for(auto& i : dataList){
        date = i.OHLCDataTime;

        date.erase(date.begin(), std::find_if(date.begin(), date.end(), [](unsigned char ch) {return !std::isspace(ch);}));

        i.OHLCDataTime = date;

        day = stoi(date.substr(0, 2));

        std::tm then{};

        std::istringstream in(date);

        if(!(in >> std::get_time(&then, "%d-%m-%Y")))
            std::cerr << "Conversion failed\n";

        mktime(&then);

        // flag("b4");
        char buffer[10];
        std::strftime(buffer, sizeof(buffer), "%V", &then);
        //flag("b5");
        week = std::stoi(buffer);

        i.weekNo = week;
        i.dayNo = day;
    }
}

int main(){

    string file_path = "nifty_50_formatted.csv";

    ifstream file(file_path);

    if(!file.is_open()){
        cerr << "Could not open file" << endl;
        return 1;
    }
    else{
        cout << "able to access the file" << endl;
    }

    string line, word;
    vector<OHLCData> dataList;

    getline(file, line);

    while(getline(file, line)){
        stringstream ss(line);
        OHLCData row;

        // flag("a");

        getline(ss, row.ticker, ',');
        getline(ss, row.OHLCDataTime, ',');

        getline(ss, word, ','); row.open = stod(word);
        getline(ss, word, ','); row.high = stod(word);
        getline(ss, word, ','); row.low = stod(word);
        getline(ss, word, ','); row.close = stod(word);

        dataList.push_back(row);
    }

    file.close();

    // flag("b");
    addWeeknoDayno(dataList);

    sort(dataList.begin(), dataList.end(), sorting_function);

    ll mn = min(dataList.size(), 100);

    // flag("c");

    cout << "print_OHLCData" << endl;
    for(ll i =0 ; i < mn; i++){
        print_OHLCData(dataList[i]);
    }
    cout << endl << endl;

    ll count = 0, consecutiveRed = 3;
    ll dls = dataList.size();
    bool B = 0;
    Trade temp;
    vector<Trade> ans;

    for(ll i =0; i < dls; i++){
        if(B){
            if(dataList[i - 1].close < dataList[i].close){
                temp.exitVal = dataList[i].close;
                temp.exitOHLCDataTime = dataList[i].OHLCDataTime;
                B = 0;
                ans.push_back(temp);
            }
        }
        else{
            if(dataList[i].open > dataList[i].close){
                count++;
                if(count == consecutiveRed){
                    B = 1;
                    temp.entryVal = dataList[i].close;
                    temp.entryOHLCDataTime = dataList[i].OHLCDataTime;
                    count = 0;
                }
            }
            else{
                count = 0;
            }
        }
    }

    cout << "Trades: " << endl;
    for(auto& i: ans){
        cout << i.entryOHLCDataTime << " " << i.exitOHLCDataTime << endl;
    }

    return 0;
}