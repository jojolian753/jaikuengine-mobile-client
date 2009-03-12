import timehistogram
import timeplots

import datetime

start = timehistogram.beginningOfDay( datetime.datetime.today() )
end = timehistogram.beginningOfDay( start + datetime.timedelta(99) )

h = timehistogram.DayHistogram(start, end)
for t in range(0,100):
    h.values.append( int(7 * (t % 3) + (t / 11) + (t*t)/1000 ) )
print h.values


ax = timeplots.begin('Warnings')
timeplots.plotHistogram(0, h, cumulative=False)
timeplots.end(ax)
timeplots.savePlot('my.png')

    
