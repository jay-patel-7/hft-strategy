#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <vector>
#include <ctime>

#define ll long long

using namespace std;

struct Trade{
    double entryVal, exitVal;
    string entryOHLCDataTime, exitOHLCDataTime;

    double pnl;
    double mae;
    double mfe;
};

struct OHLCData{
    string ticker, OHLCDataTime;
    double open, high, low, close;
    ll weekNo, dayNo;
};

bool operator<(OHLCData const& lhs, OHLCData const& rhs) noexcept{
    if(lhs.weekNo < rhs.weekNo) return true;
    if(lhs.weekNo == rhs.weekNo) return lhs.dayNo < rhs.dayNo;
    return false;
}

bool sorting_function(OHLCData& lhs, OHLCData& rhs){
    return lhs < rhs;
}

vector<OHLCData> convertToWeekly(vector<OHLCData>& daily){

    vector<OHLCData> weekly;

    if(daily.empty()) return weekly;

    OHLCData currentWeek = daily[0];

    currentWeek.open = daily[0].open;
    currentWeek.high = daily[0].high;
    currentWeek.low  = daily[0].low;
    currentWeek.close = daily[0].close;

    for(size_t i = 1; i < daily.size(); i++){

        if(daily[i].weekNo == currentWeek.weekNo){
            currentWeek.high = max(currentWeek.high, daily[i].high);
            currentWeek.low  = min(currentWeek.low, daily[i].low);
            currentWeek.close = daily[i].close;
        }
        else{
            weekly.push_back(currentWeek);

            currentWeek = daily[i];
            currentWeek.open = daily[i].open;
            currentWeek.high = daily[i].high;
            currentWeek.low  = daily[i].low;
            currentWeek.close = daily[i].close;
        }
    }

    weekly.push_back(currentWeek);
    return weekly;
}

void addWeeknoDayno(vector<OHLCData>& dataList){
    time_t base_time = 0;
    bool first = true;
    int offset = 0;

    for(auto& i : dataList){
        string date = i.OHLCDataTime;

        date.erase(date.begin(), std::find_if(date.begin(), date.end(),
            [](unsigned char ch) { return !std::isspace(ch); }));

        i.OHLCDataTime = date;

        int day = stoi(date.substr(0, 2));
        int month = stoi(date.substr(3, 2));
        int year = stoi(date.substr(6, 4));

        std::tm then{};
        then.tm_mday = day;
        then.tm_mon = month - 1;
        then.tm_year = year - 1900;

        time_t current = mktime(&then);

        if(first){
            base_time = current;
            first = false;
            offset = (then.tm_wday == 0 ? 7 : then.tm_wday);
        }

        int days_diff = (current - base_time) / (60 * 60 * 24);
        int week = (days_diff + offset) / 7 + 1;

        int dayNo = (then.tm_wday == 0 ? 7 : then.tm_wday);

        i.weekNo = week;
        i.dayNo = dayNo;
    }
}

int main(){

    string file_path = "nifty_50_formatted.csv";

    ifstream file(file_path);

    if(!file.is_open()){
        cerr << "Could not open file" << endl;
        return 1;
    }

    string line, word;
    vector<OHLCData> dataList;

    getline(file, line); // skip header

    while(getline(file, line)){
        stringstream ss(line);
        OHLCData row;

        getline(ss, row.ticker, ',');
        getline(ss, row.OHLCDataTime, ',');

        try {
            getline(ss, word, ','); row.open = stod(word);
            getline(ss, word, ','); row.high = stod(word);
            getline(ss, word, ','); row.low = stod(word);
            getline(ss, word, ','); row.close = stod(word);
        } catch(...) {
            continue;
        }

        dataList.push_back(row);
    }

    file.close();

    addWeeknoDayno(dataList);

    sort(dataList.begin(), dataList.end(), sorting_function);

    vector<OHLCData> workingData = convertToWeekly(dataList);

    ll consecutiveRed = 3;
    ll wds = workingData.size();

    bool inTrade = false;

    Trade temp;
    vector<Trade> ans;

    ll redCount = 0;

    int upcloseTarget = 3;   // 🔥 dynamic exit
    int upcloseCount = 0;

    double lowestLow = 0;
    double highestHigh = 0;

    for(ll i = 1; i < wds; i++){

        if(inTrade){

            // update MAE / MFE trackers
            lowestLow = min(lowestLow, workingData[i].low);
            highestHigh = max(highestHigh, workingData[i].high);

            // check upclose
            if(workingData[i].close > workingData[i - 1].close){
                upcloseCount++;
            }

            // exit condition
            if(upcloseCount == upcloseTarget){
                temp.exitVal = workingData[i].close;
                temp.exitOHLCDataTime = workingData[i].OHLCDataTime;

                temp.pnl = temp.exitVal - temp.entryVal;
                temp.mae = lowestLow - temp.entryVal;
                temp.mfe = highestHigh - temp.entryVal;

                ans.push_back(temp);

                inTrade = false;
                upcloseCount = 0;
            }
        }
        else{
            // check red candle
            if(workingData[i].open > workingData[i].close){
                redCount++;

                if(redCount == consecutiveRed){
                    inTrade = true;

                    temp = Trade();
                    temp.entryVal = workingData[i].close;
                    temp.entryOHLCDataTime = workingData[i].OHLCDataTime;

                    lowestLow = workingData[i].low;
                    highestHigh = workingData[i].high;

                    upcloseCount = 0;
                    redCount = 0;
                }
            }
            else{
                redCount = 0;
            }
        }
    }

    ofstream outFile("trades.txt");

    if(!outFile.is_open()){
        cerr << "error opening trades.txt\n";
    }
    else{
        outFile << "EntryDate,EntryPrice,ExitDate,ExitPrice,PnL,MAE,MFE\n";

        for(auto& i: ans){
            outFile << i.entryOHLCDataTime << ","
                    << i.entryVal << ","
                    << i.exitOHLCDataTime << ","
                    << i.exitVal << ","
                    << i.pnl << ","
                    << i.mae << ","
                    << i.mfe << "\n";
        }

        outFile.close();
        cout << "Trades saved to trades.txt\n";
    }

    return 0;
}