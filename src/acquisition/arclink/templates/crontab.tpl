#sync_minute# #sync_hour# #sync_day# * 0 #pkgroot#/operator/autosync -s -f >/dev/null 2>&1
#sync_minute# #sync_hour# * * 1-6 #pkgroot#/operator/autosync -s >/dev/null 2>&1
