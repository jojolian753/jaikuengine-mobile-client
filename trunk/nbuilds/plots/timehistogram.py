# TimeHistogram -  histograms over time ranges. 

import datetime


def DayHistogram(first, last):
    return TimeHistogram(first, last, datetime.timedelta(days=1))

def FlatHistogram(total, first, last):
    h = TimeHistogram(first, last, datetime.timedelta(days=1))
    h.total = total
    return h

class TimeHistogram:
    def __init__(self, first, last, binsize):
        self.binsize = binsize # FIXME: rename
        self.start = first
        self.end = last + binsize
        self.values = []

    def total(self):
        if not self.total:
            total = 0 
            for v in self.values:
                total += v
            return total
        else:
            self.total

    def cumulative(self):
        sum = []
        total = 0 
        for v in self.values:
            total += v
            sum.append(total)
        return sum


# Some utility methods 
def beginningOfDay(dt):
    return datetime.datetime(dt.year, dt.month, dt.day)

def inclusiveDayRange(t1, t2):
    t2 = t2 + datetime.timedelta(days=1)
    s = beginningOfDay(t1)
    e = beginningOfDay(t2)
    return (s,e)

def historicalFlat(total, start, end):
    start = beginningOfDay( start )
    end   = beginningOfDay( end )

    histogram = DayHistogram(start, end)

    dailyAvg = total / (histogram.end - histogram.start).days
    histogram.values = []

    t = histogram.start
    while t < histogram.end:
        histogram.values.append( dailyAvg )
        t += histogram.binsize

    # put extra 
    sum = 0
    for v in histogram.values: sum += v
    remainder = total - sum
    for i in range(remainder):
        histogram.values[i] += 1
        
    return histogram
    
    
def countsBinnedByDay(tstamps, start = None, end = None):    
    # first we calculate counts with dictionary
    counts = {}
    for t in tstamps:
        day = beginningOfDay(t)
        if not counts.has_key(day):
            counts[day] = 1
        else:
            counts[day] = counts[day] + 1

    # then we sort dates with values
    valuedDates = counts.keys()
    valuedDates.sort()

    # then we create histogram

    if not start: start = beginningOfDay( valuedDates[0] )
    if not end:   end   = beginningOfDay( valuedDates[-1] )

    histogram = DayHistogram(start, end)

    histogram.values = []

    t = histogram.start
    while t < histogram.end:
        if counts.has_key(t):
            histogram.values.append( counts[t] )
        else:
            histogram.values.append( 0 )        
        t += histogram.binsize
            
    return histogram
