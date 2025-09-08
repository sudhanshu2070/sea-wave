// OHLC.h
// Simple struct for OHLC data
#ifndef OHLC_H
#define OHLC_H

struct OHLC {
    long time;
    double open;
    double high;
    double low;
    double close;
    double volume;
};

#endif