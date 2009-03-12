import matplotlib
matplotlib.use('Agg')

from pylab import *
import datetime

import timehistogram

myBlack = '#000000'
myLightGray = '#dddddd'
myGray = '#888888'
myDarkGray = '#333333'
myRed  = '#ff0000'

smallFont = 8

def markDate(bin, level):
    # draw a line]
    fill([bin, bin], [0, level + smallFont + 5], edgecolor=myLightGray)
    
    text(bin, level, num2date(bin).strftime("%d.%m.%Y"),
         size=smallFont,
         color=myLightGray,
         horizontalalignment='right')



def drangeFromHistogram(histogram):
    return drange(histogram.start, histogram.end, histogram.binsize)

def label(x, y, alignment='center'):
    text(x, y + 1, str(y), color=myRed, size=smallFont,
         horizontalalignment=alignment)

def labelLast(xs,ys):
    x = xs[-1]
    y = ys[-1]
    label(x,y,'right')
   
def emphasize(x,y):
    plot_date([x], [y], '.r')

def emphasizeLast(xs, ys):
    x = xs[-1]
    y = ys[-1]
    emphasize(x,y)
    
def barplot(bins, histogram):
    v = histogram.values
    delta = bins[1]-bins[0]
    
    def xline(x):
        return (x, x+delta)
    count_data = len(v)
    min_data = min(v)
    max_data = max(v)
    min_index = v.index(min_data)
    max_index = v.index(max_data)

    bar(bins, v, color=myLightGray)

    max_day = bins[max_index]
    max_value = v[max_index]

    label(max_day, max_value)
    

def earlierThanToday(date):
    today = datetime.datetime.today()
    d = datetime.datetime(date.year, date.month, date.day)
    return d < today


def cumulativePlot(base, bins, histogram):
    sums = histogram.cumulative()
    sums = [s+base for s in sums]
    end = histogram.end 
    if earlierThanToday(end):
        end = datetime.datetime.today()
        bins = drange(histogram.start, end, histogram.binsize)
        latest = sums[-1]
        diff = len(bins) - len(sums)
        sums += [latest for r in range(diff)] 

    plot_date(bins, sums , '-', color=myGray)

    emphasizeLast(bins, sums)
    labelLast(bins, sums)
    markDate( bins[-1], 10 )
    
#     for i in range(count_data):
#         color = '#999999'
#         linewidth = 3
#         plot( (bins[i],bins[i]+delta), (0, v[i]), color=color)
        
#     # color max differently 
#     color = '#dd0000'
#     plot( (max_index, max_index), (0, max_data), color=color, linewidth=linewidth)
  
def begin(titlestr):
    rc('grid', color='#00FF00', linestyle='-')
    
    f = figure(figsize=(8,4))
    ax = subplot(111)
    title(titlestr)
    return ax 

def end(ax):
    months   = MonthLocator()  # every month
    monthsFmt = DateFormatter('%B %Y')
    
    ax.xaxis.set_major_locator(months)
    ax.xaxis.set_major_formatter(monthsFmt)
    ax.fmt_xdata = monthsFmt
    
    maxV = axis()[3]
    maxV += 100
    maxV = (int(maxV) / 100) * 100
    axis( list(axis())[0:2] + [0, maxV])


    
def plotHistorical(histogram):
    bins = drange(histogram.start, histogram.end, histogram.binsize)
    flattotal = [histogram.total for b in bins]
    plot_date(bins, flattotal , '-', color=myGray)

    labelLast(bins,flattotal)
    emphasizeLast(bins, flattotal)

    middle = len(bins)/2
    dt = histogram.end.strftime("%d.%m.%Y")
    text(bins[middle], flattotal[middle] + 30, 
         "Timestamps not available\nbefore %s" % (dt),
         horizontalalignment='center',
         size=smallFont,
         color=myLightGray)
    

def plotHistogram(base, histogram, cumulative=True):
    bins = drange(histogram.start, histogram.end, histogram.binsize)
    print len(bins)
    barplot(bins, histogram)
    if cumulative:
        cumulativePlot(base, bins, histogram)
    
def showPlot():
    show()

def savePlot(file):
    savefig(file)



# Activity traces
#
# 
# teemu   -- -      - ----   - --      ---
# mikie    -- ---       ---    --        - --
# darla           - -               --
#                | Sep 19         | Sep 20
#
# for each dot: x=timestamp, y=(userpos*ROWHEIGHT)
# 
# Essentially 2d data:
# Data in format
# nick, [timestamps]
#
# for historical data, where we have only connections starting and endings
# data is in format
# nick, [ (start tstamp, end tstamp) ]
# 
# Data can be sorted so that most active users are on top.
# We can do also levelled grouping:
# 1) first are users with activity during today (sorted by overall activity)
# 2) then are users with activity during last week (sorted by overall activity)
# 3) then are users with activity during last month ( -- "" -- )
# 4) then are rest of users


def plotBinnedActivityTrace(trace, orderedNicks):
    HEIGHT = 1
    npos = HEIGHT
    for n in orderedNicks:
        h = trace.binned[n]
        # bins should be equal accross all nicks
        bins = drange(h.start, h.end, h.binsize) 

        ys = []
        for v in h.values:
            y = 0
            if v > 0:
                y = npos
            ys.append(y)
            npos += HEIGHT
        plot_date( bins,  ys, ",r" )
