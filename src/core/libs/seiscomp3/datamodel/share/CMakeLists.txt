SET(
    DATAMODEL_SHARE_DATA
	sqlite3.sql
	mysql.sql
	postgres.sql
	update.py
)

SC_INSTALL_DATA(DATAMODEL_SHARE db)
