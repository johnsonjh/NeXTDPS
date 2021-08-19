#  etc/hourly.bsd
#
# hourly care and feeding of PostScript printers on BSD systems
# this is a template run by cron with the following crontab entry
# 17 * * * * sh < /usr/adm/hourly.ctl >/usr/adm/hourly.log 2>&1

printers="LIST OF POSTSCRIPT PRINTERS"
HFILES="PSLIBDIR/ehandler.ps PSLIBDIR/uartpatch.ps"
for p in $printers
do
	(lpq -P$p | fgrep ehandler.ps >/dev/null) || lpr -P$p $HFILES
done
