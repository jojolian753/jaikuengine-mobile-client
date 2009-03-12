library(DBI)
library(ROracle)
ora <- dbDriver("Oracle")
con <- dbConnect(ora, "context/nSrXt8p@context.cs.helsinki.fi")
res <- dbSendQuery(con, "\
select \
	call.datetime,\
	case act.state when 'active' then 0\
	else 1440*(call.datetime - act.valid_from - 1/1440) end t,\
	call.direction, act.state, act.person\
from\
	activity act, call, profile prof\
where\
	act.person = call.person and\
	act.person = prof.person and\
	call.datetime - 1/1440 between act.valid_from and act.valid_until and\
	call.datetime - 1/1440 between prof.valid_from and prof.valid_until and\
	( call.direction = 'Incoming' or call.direction = 'Missed call' )\
	and not exists ( select call2.person from call call2 where\
		call.person = call2.person and\
		call.datetime between call2.datetime+1/(60*1440) and\
			call2.datetime+call2.duration/(60*1440) and\
		call2.duration>0\
	)\
	and not prof.id = 4\
")
chunk <- fetch(res, n=10000)
missed <- chunk[chunk[,3]=="Missed call",2]
incoming <- chunk[chunk[,3]=="Incoming",2]

plot(density(log(missed+1)), col="red")
lines(density(log(incoming+1)), col="blue")

h<-c(0.0, 1, 5, 10, 15, 30, 60, 120, 300, 1000, 5000, 10000, 30000)
x <- h
y <- h; yc <- h
counts <- h; countsc <- h
for (i in 1:length(h))
{
	if (i==1) {
		low <- -1
	} else {
		low <- h[i-1]
	}
	m <- sum( missed > low & missed <= h[i] )
	inc <- sum( incoming > low & incoming <= h[i] )
	counts[i] <- m+inc
	y[i] <- m/(m+inc)
	mc <- sum( missed <= h[i] )
	incc <- sum( incoming <= h[i] )
	countsc[i] <- mc+incc
	yc[i] <- mc/(mc+incc)
}
a <- matrix(nrow=length(h), ncol=3)
a[,1]=h; a[,2]=y; a[,3]=counts
ac <- matrix(nrow=length(h), ncol=3)
ac[,1]=h; ac[,2]=yc; ac[,3]=countsc
plot(x[x<1100], y[x<1100], "l", log="x")
lines(x[x<1100], yc[x<1100], "l", log="x", col="blue")
plot(x, y, "l")

min_t<-0
max_t<-max(max(missed), max(incoming))
c<-10
x<- 1:c
y<- 1:c
l<- 1:c
h<- 1:c
n<- 1:c
for (i in 1:c) {
	low <- exp( (i-1) * log(max_t)/c)-1
	l[i] <- low
	high <- exp( (i+1) * log(max_t)/c)
	h[i] <- high
	x[i] <- exp(i * log(max_t)/c)
	miss <- sum(missed >= low & missed <= high)
	inc <- sum(incoming >= low & incoming <= high)
	n[i] <- miss+inc
	y[i] <- miss/(miss+inc)
}
plot(x, y, "l", log="x")
