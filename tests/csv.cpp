#include "../src/csv_columns.hpp"
#include <cassert>
int main(){
    assert(columnIs("TimeInQPC","timeinqpc"));
    assert(columnIs("QPCTime","qpctime"));
    assert(columnIs("MsBetweenPresents","msbetweenpresents"));
    assert(columnIs("msBetweenPresents","msbetweenpresents"));
    assert(columnIs(" \"ProcessID\" ","processid"));
    assert(!columnIs("TimeInMs","timeinqpc"));
}
