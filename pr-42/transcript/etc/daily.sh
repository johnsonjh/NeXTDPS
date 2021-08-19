# etc/daily.bsd
#
# this code is included as part of our daily.sh file
# which is invoked by cron(8) with the following crontab
# entry:
# 1 0 * * * sh </usr/adm/daily.sh >/usr/adm/daily.log

printers="LIST OF POSTSCRIPT PRINTERS'
for p in $printers
do
	# rotate log files
	cp LOGDIR/PRINTER-log	LOGDIR/PRINTER-log.1
	cp /dev/null		LOGDIR/PRINTER-log

	# summarize printer accounting
	/etc/pac -PPRINTER -s
done
