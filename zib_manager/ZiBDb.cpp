#include "stdafx.h"
#include "ZiBDb.h"
#include <ibpp/core/ibpp.h>

#include <stdio.h>
#include <string>
#include <sstream>

// ************************************
// * Valid Types                      *
// ************************************
// C++				|	Firebird
// -----------------------------------
// bool				|
// std::string		|	CHAR / VARCHAR
// int16_t			|	SMALLINT
// int32_t			|	INTEGER
// int64_t			|	BIGINT
// float			|	FLOAT
// double			|	DOUBLE PRECISION
// IBPP::Timestamp	|	TIMESTAMP
// IBPP::Date		|	DATE
// IBPP::Time		|	TIME
// IBPP::Blob		|	BLOB
// IBPP::Array		|	ARRAY
// ??? DBKey ???	|

CCriticalSection CZiBDb::lock::m_cs;

CZiBDb::CZiBDb()
{
}

CZiBDb::~CZiBDb()
{
	// m_db->Disconnect(); // not needed at all... m_db destructor should handle
}

void CZiBDb::Setup(const std::string &strServerName, 
		const std::string &strUserName, 
		const std::string &strPassword, 
		const std::string &strDatabaseName, 
		const std::string &strBackupName,
		const std::string &strWriteMode)
{
	lock locker;
	m_strServerName = strServerName;
	m_strUserName = strUserName;
	m_strPassword = strPassword;
	m_strDatabaseName = strDatabaseName;
	m_strBackupName = strBackupName;

	if(strWriteMode =="speed")
	{
		m_iWriteMode = 1;
	}
	else if(strWriteMode =="safety")
	{
		m_iWriteMode = 2;
	}
	else
	{
		m_iWriteMode = 0;
	}

	m_db = IBPP::DatabaseFactory(m_strServerName, 
		m_strDatabaseName, 
		m_strUserName, 
		m_strPassword,
		"", 
		"WIN1252", 
		"PAGE_SIZE 8192 DEFAULT CHARACTER SET WIN1252");
}

// true - connected
// false - file does not exist
bool CZiBDb::Connect(void)
{
	lock locker;

	bool bRet = false;

	try
	{
		m_db->Connect();
		bRet = true;
	}
	catch(IBPP::Exception& e)
	{
		TRACE(DBG_LVL_DEBUG, "CZiBDb::Connect - IBPP::Exception occured:\n");
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, e.what());
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, "\n");
		
		IBPP::SQLException* pe = dynamic_cast<IBPP::SQLException*>(&e);
		if (pe == 0)
			TRACE(DBG_LVL_DBG4, "CZiBDb::Connect - Not an engine error\n");
		else 
			TRACE(DBG_LVL_DBG4, "CZiBDb::Connect - Engine Code : %d\n", pe->EngineCode());

		if (pe->EngineCode() == 335544344) // todo: use a define
		{
			TRACE(DBG_LVL_DBG4,"CZiBDb::Connect - THE FILE DOES NOT EXIST\n");
		}
	}
	catch(std::exception& e)
	{
		TRACE(DBG_LVL_DEBUG, "CZiBDb::Connect - std::exception occured (%s):\n", typeid(e).name());
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, e.what());
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, "\n");
	}
	catch(...)
	{
		TRACE(DBG_LVL_DEBUG, "CZiBDb::Connect - A system exception occured\n");
	}

	return bRet;
}

/*
f()
{
	try
	{

	}
	catch(IBPP::Exception& e)
	{
		std::cout << e.what() << "\n";
		
		IBPP::SQLException* pe = dynamic_cast<IBPP::SQLException*>(&e);
		
		if (pe == 0)
			std::cout<< "Not an engine error\n";
		else 
			std::cout<< "Engine Code : "<< pe->EngineCode()<< "\n";
	}
	catch(std::exception& e)
	{
		std::cout << e.what() << "\n";

		TRACE(DBG_LVL_DBG4,"A std::exception '%s' occured !\n\n",
			typeid(e).name());
	}
	catch(...)
	{
		TRACE(DBG_LVL_DBG4,"A system exception occured!\n\n");
	}
}
*/

// a create connects
bool CZiBDb::CreateDatabase(void)
{
	lock locker;

	bool bSuccess = true;

	TRACE(DBG_LVL_DBG4,"Creating ZiB Database\n");

	int Major, Minor, PageSize, Pages, Buffers, Sweep;
	bool Sync, Reserve;

	WIN32_FIND_DATA find_data;
	if( FindFirstFile(m_strDatabaseName.c_str(), &find_data) != INVALID_HANDLE_VALUE )
	{
		TRACE(DBG_LVL_DEBUG,"The database already exists\n");
		return true;
	}
	
	DeleteFile(m_strDatabaseName.c_str());
	
	try
	{
		m_db->Create(3);		// 3 is the dialect of the database (could have been 1)

		IBPP::Service svc = IBPP::ServiceFactory(m_strServerName, m_strUserName, m_strPassword);
		svc->Connect();
		svc->SetPageBuffers(m_strDatabaseName, 256);	// Instead of default 2048
		svc->SetSweepInterval(m_strDatabaseName, 5000);	// instead of 20000 by default
		if (m_iWriteMode == 1) svc->SetSyncWrite(m_strDatabaseName, false);
		else if (m_iWriteMode == 2) svc->SetSyncWrite(m_strDatabaseName, true);
		svc->SetReadOnly(m_strDatabaseName, false);	// That's the default anyway
		svc->Disconnect();

		m_db->Connect();	// a create does no imply a connection
		m_db->Info(&Major, &Minor, &PageSize, &Pages, &Buffers, &Sweep, &Sync, &Reserve);
		if (Sync && m_iWriteMode == 1)
		{
			bSuccess = false;
			TRACE(DBG_LVL_DEBUG,_("The created database has sync writes enabled (safety),\n"
				"while it was expected to be disabled (speed).\n"));
		}

		if (! Sync && m_iWriteMode == 2)
		{
			bSuccess = false;
			TRACE(DBG_LVL_DEBUG,_("The created database has sync writes disabled (speed),\n"
				"while it was expected to be enabled (safety).\n"));
		}

		if (Sync) TRACE(DBG_LVL_DEBUG,_("           Sync Writes is enabled (Safety).\n"
			"           Use 'speed' command-line argument to test the other mode.\n"));
		else TRACE(DBG_LVL_DEBUG,_("           Sync Writes is disabled (Speed).\n"
			"           Use 'safety' command-line argument to test the other mode.\n"));

		TRACE(DBG_LVL_DEBUG,"           ODS Major %d\n", Major);
		TRACE(DBG_LVL_DEBUG,"           ODS Minor %d\n", Minor);
		TRACE(DBG_LVL_DEBUG,"           Page Size %d\n", PageSize);
		TRACE(DBG_LVL_DEBUG,"           Pages     %d\n", Pages);
		TRACE(DBG_LVL_DEBUG,"           Buffers   %d\n", Buffers);
		TRACE(DBG_LVL_DEBUG,"           Sweep     %d\n", Sweep);
		TRACE(DBG_LVL_DEBUG,"           Reserve   %s\n", Reserve ? _("true") : _("false"));
	}
	catch(IBPP::Exception& e)
	{
		TRACE(DBG_LVL_DEBUG, "CZiBDb::CreateDatabase - IBPP::Exception occured:\n");
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, e.what());
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, "\n");
		
		IBPP::SQLException* pe = dynamic_cast<IBPP::SQLException*>(&e);
		if (pe == 0)
			TRACE(DBG_LVL_DBG4, "CZiBDb::CreateDatabase - Not an engine error\n");
		else 
			TRACE(DBG_LVL_DBG4, "CZiBDb::CreateDatabase - Engine Code : %d\n", pe->EngineCode());

		if (pe->EngineCode() == 335544344) // todo: use a define
		{
			TRACE(DBG_LVL_DBG4,"CZiBDb::CreateDatabase - THE FILE DOES NOT EXIST\n");
		}
		bSuccess = false;
	}
	catch(std::exception& e)
	{
		TRACE(DBG_LVL_DEBUG, "CZiBDb::CreateDatabase - std::exception occured (%s):\n", typeid(e).name());
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, e.what());
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, "\n");
		bSuccess = false;
	}
	catch(...)
	{
		TRACE(DBG_LVL_DEBUG, "CZiBDb::CreateDatabase - A system exception occured\n");
		bSuccess = false;
	}

	return bSuccess;
}

bool CZiBDb::DropDatabase(void)
{
	lock locker;
	bool bRet = false;

	TRACE(DBG_LVL_DBG4,"Dropping ZiB Database\n");

	try
	{
		if(m_db->Connected())	
		{
			m_db->Drop();
			TRACE(DBG_LVL_DEBUG,"Success\n");
			bRet = true;;
		}
		else
		{
			TRACE(DBG_LVL_DEBUG,"Failure: Not connected to the database\n");
		}
	}
	catch(IBPP::Exception& e)
	{
		TRACE(DBG_LVL_DEBUG, "CZiBDb::DropDatabase - IBPP::Exception occured:\n");
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, e.what());
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, "\n");
		
		IBPP::SQLException* pe = dynamic_cast<IBPP::SQLException*>(&e);
		if (pe == 0)
			TRACE(DBG_LVL_DBG4, "CZiBDb::DropDatabase - Not an engine error\n");
		else 
			TRACE(DBG_LVL_DBG4, "CZiBDb::DropDatabase - Engine Code : %d\n", pe->EngineCode());

		if (pe->EngineCode() == 335544344) // todo: use a define
		{
			TRACE(DBG_LVL_DBG4,"CZiBDb::DropDatabase - THE FILE DOES NOT EXIST\n");
		}
	}
	catch(std::exception& e)
	{
		TRACE(DBG_LVL_DEBUG, "CZiBDb::DropDatabase - std::exception occured (%s):\n", typeid(e).name());
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, e.what());
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, "\n");
	}
	catch(...)
	{
		TRACE(DBG_LVL_DEBUG, "CZiBDb::DropDatabase - A system exception occured\n");
	}

	return bRet;
}


bool CZiBDb::CreateTables(void)
{
	lock locker;
	bool bRet = false;

	TRACE(DBG_LVL_DBG4,"CZiBDb::CreateTables\n");

	try
	{
		if(m_db->Connected())	
		{
			// The following transaction configuration values are the defaults and
			// those parameters could have as well be omitted to simplify writing.
			IBPP::Transaction tr1 = IBPP::TransactionFactory(m_db,
									IBPP::amWrite, IBPP::ilConcurrency, IBPP::lrWait);
			tr1->Start();

			IBPP::Statement st1 = IBPP::StatementFactory(m_db, tr1);

			st1->ExecuteImmediate(	
				"CREATE TABLE t_nodes ("
				"mac_addr BIGINT NOT NULL,"
				"nwk_addr SMALLINT,"
				"profile_id SMALLINT,"
				"device_id SMALLINT,"
				"online SMALLINT,"
				"tag VARCHAR(50),"
				"PRIMARY KEY (mac_addr) );");
			tr1->CommitRetain();

			st1->ExecuteImmediate(	
				"CREATE TABLE t_attributes ("
				"mac_addr BIGINT NOT NULL,"
				"ep_number SMALLINT NOT NULL,"
				"cluster_id SMALLINT NOT NULL,"
				"attribute_id SMALLINT NOT NULL,"
				"attribute_value VARCHAR(50),"
				"up_limit VARCHAR(50),"
				"down_limit VARCHAR(50),"
				"limit_action SMALLINT,"
				"FOREIGN KEY (mac_addr) REFERENCES t_nodes (mac_addr) );" );
			tr1->CommitRetain();

			st1->ExecuteImmediate(	
				"CREATE TABLE t_attributes_history ("
				"mac_addr BIGINT NOT NULL,"
				"ep_number SMALLINT NOT NULL,"
				"cluster_id SMALLINT NOT NULL,"
				"attribute_id SMALLINT NOT NULL,"
				"measure_time TIMESTAMP NOT NULL,"
				"attribute_value VARCHAR(50), "
				"attribute_value_double DOUBLE PRECISION, "
				"FOREIGN KEY (mac_addr) REFERENCES t_nodes (mac_addr) );" );
			tr1->CommitRetain();


			bRet = true;
		}
		else
		{
			TRACE(DBG_LVL_DEBUG,"Failure: Not connected to the database\n");
		}
	}
	catch(IBPP::Exception& e)
	{
		TRACE(DBG_LVL_DEBUG, "CZiBDb::CreateTables - IBPP::Exception occured:\n");
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, e.what());
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, "\n");
		
		IBPP::SQLException* pe = dynamic_cast<IBPP::SQLException*>(&e);
		if (pe == 0)
			TRACE(DBG_LVL_DBG4, "CZiBDb::CreateTables - Not an engine error\n");
		else 
			TRACE(DBG_LVL_DBG4, "CZiBDb::CreateTables - Engine Code : %d\n", pe->EngineCode());

		if (pe->EngineCode() == 335544344) // todo: use a define
		{
			TRACE(DBG_LVL_DBG4,"CZiBDb::CreateTables - THE FILE DOES NOT EXIST\n");
		}
	}
	catch(std::exception& e)
	{
		TRACE(DBG_LVL_DEBUG, "CZiBDb::DropDatabase - std::exception occured (%s):\n", typeid(e).name());
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, e.what());
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, "\n");
	}
	catch(...)
	{
		TRACE(DBG_LVL_DEBUG, "CZiBDb::CreateTables - A system exception occured\n");
	}

	return bRet;
}

bool CZiBDb::InsertNode(ZiBNodeRow &node)
{
	lock locker;

	return InsertNode(node.GetMacAddr(),
		node.GetNwkAddr(),
		node.GetProfileId(),
		node.GetDeviceId(),
		node.GetOnline() );
}

bool CZiBDb::InsertNode(int64_t qwMacAddr,
   int16_t wNwkAddr, 
   int16_t wProfileId, 
   int16_t wDeviceId, 
   bool bOnline)
{
	lock locker;
	bool bRet = false;

	TRACE(DBG_LVL_DBG4,"CZiBDb::InsertNode\n");

	try
	{
		if(m_db->Connected())	
		{
			IBPP::Transaction tr1 = IBPP::TransactionFactory(m_db);
			tr1->Start();

			IBPP::Statement st1 = IBPP::StatementFactory(m_db, tr1);
			st1->Prepare(	"INSERT INTO t_nodes (mac_addr, nwk_addr, profile_id, device_id, online)"
							"VALUES (?, ?, ?, ?, ?);" );
			st1->Set(1, qwMacAddr);
			st1->Set(2, wNwkAddr);
			st1->Set(3, wProfileId);
			st1->Set(4, wDeviceId);
			st1->Set(5, bOnline);
			st1->Execute();

			tr1->CommitRetain();

			bRet =  true;
		}
		else
		{
			TRACE(DBG_LVL_DEBUG,"Failure: Not connected to the database\n");
		}
	}
	catch(IBPP::Exception& e)
	{
		TRACE(DBG_LVL_DEBUG, "CZiBDb::InsertNode - IBPP::Exception occured:\n");
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, e.what());
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, "\n");
		
		IBPP::SQLException* pe = dynamic_cast<IBPP::SQLException*>(&e);
		if (pe == 0)
			TRACE(DBG_LVL_DBG4, "CZiBDb::InsertNode - Not an engine error\n");
		else 
			TRACE(DBG_LVL_DBG4, "CZiBDb::InsertNode - Engine Code : %d\n", pe->EngineCode());

		if (pe->EngineCode() == 335544344) // todo: use a define
		{
			TRACE(DBG_LVL_DBG4,"CZiBDb::InsertNode - THE FILE DOES NOT EXIST\n");
		}
	}
	catch(std::exception& e)
	{
		TRACE(DBG_LVL_DEBUG, "CZiBDb::InsertNode - std::exception occured (%s):\n", typeid(e).name());
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, e.what());
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, "\n");
	}
	catch(...)
	{
		TRACE(DBG_LVL_DEBUG, "CZiBDb::InsertNode - A system exception occured\n");
	}

	return bRet;
}

bool CZiBDb::UpdateNode(ZiBNodeRow &node)
{
	lock locker;

	return UpdateNode(node.GetMacAddr(),
		node.GetNwkAddr(),
		node.GetProfileId(),
		node.GetDeviceId(),
		node.GetOnline());
}

bool CZiBDb::UpdateNode(int64_t qwMacAddr,
	int16_t wNwkAddr, 
	int16_t wProfileId, 
	int16_t wDeviceId, 
	bool bOnline)
{
	lock locker;
	bool bRet = false;

	TRACE(DBG_LVL_DBG4,"CZiBDb::UpdateNode\n");

	try
	{
		if(m_db->Connected())	
		{
			IBPP::Transaction tr1 = IBPP::TransactionFactory(m_db);
			tr1->Start();

			IBPP::Statement st1 = IBPP::StatementFactory(m_db, tr1);
			st1->Prepare(	"UPDATE t_nodes SET nwk_addr = ?, profile_id = ?, device_id = ?, online = ? "
							"WHERE mac_addr = ?;");
			st1->Set(1, wNwkAddr);
			st1->Set(2, wProfileId);
			st1->Set(3, wDeviceId);
			st1->Set(4, bOnline);
			st1->Set(5, qwMacAddr);
			st1->Execute();

			tr1->CommitRetain();

			bRet = true;
		}
		else
		{
			TRACE(DBG_LVL_DEBUG,"Failure: Not connected to the database\n");
		}
	}
	catch(IBPP::Exception& e)
	{
		TRACE(DBG_LVL_DEBUG, "CZiBDb::UpdateNode - IBPP::Exception occured:\n");
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, e.what());
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, "\n");
		
		IBPP::SQLException* pe = dynamic_cast<IBPP::SQLException*>(&e);
		if (pe == 0)
			TRACE(DBG_LVL_DBG4, "CZiBDb::UpdateNode - Not an engine error\n");
		else 
			TRACE(DBG_LVL_DBG4, "CZiBDb::UpdateNode - Engine Code : %d\n", pe->EngineCode());

		if (pe->EngineCode() == 335544344) // todo: use a define
		{
			TRACE(DBG_LVL_DBG4,"CZiBDb::UpdateNode - THE FILE DOES NOT EXIST\n");
		}
	}
	catch(std::exception& e)
	{
		TRACE(DBG_LVL_DEBUG, "CZiBDb::UpdateNode - std::exception occured (%s):\n", typeid(e).name());
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, e.what());
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, "\n");
	}
	catch(...)
	{
		TRACE(DBG_LVL_DEBUG, "CZiBDb::UpdateNode - A system exception occured\n");
	}

	return bRet;
}

bool CZiBDb::UpdateNode(int64_t qwMacAddr,
	int16_t wNwkAddr, 
	int16_t wProfileId, 
	int16_t wDeviceId)
{
	lock locker;
	bool bRet = false;

	TRACE(DBG_LVL_DBG4,"CZiBDb::UpdateNode\n");

	try
	{
		if(m_db->Connected())	
		{
			IBPP::Transaction tr1 = IBPP::TransactionFactory(m_db);
			tr1->Start();

			IBPP::Statement st1 = IBPP::StatementFactory(m_db, tr1);
			st1->Prepare(	"UPDATE t_nodes SET nwk_addr = ?, profile_id = ?, device_id = ? "
							"WHERE mac_addr = ?;");
			st1->Set(1, wNwkAddr);
			st1->Set(2, wProfileId);
			st1->Set(3, wDeviceId);
			st1->Set(4, qwMacAddr);
			st1->Execute();

			tr1->CommitRetain();

			bRet = true;
		}
		else
		{
			TRACE(DBG_LVL_DEBUG,"Failure: Not connected to the database\n");
		}
	}
	catch(IBPP::Exception& e)
	{
		TRACE(DBG_LVL_DEBUG, "CZiBDb::UpdateNode - IBPP::Exception occured:\n");
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, e.what());
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, "\n");
		
		IBPP::SQLException* pe = dynamic_cast<IBPP::SQLException*>(&e);
		if (pe == 0)
			TRACE(DBG_LVL_DBG4, "CZiBDb::UpdateNode - Not an engine error\n");
		else 
			TRACE(DBG_LVL_DBG4, "CZiBDb::UpdateNode - Engine Code : %d\n", pe->EngineCode());

		if (pe->EngineCode() == 335544344) // todo: use a define
		{
			TRACE(DBG_LVL_DBG4,"CZiBDb::UpdateNode - THE FILE DOES NOT EXIST\n");
		}
	}
	catch(std::exception& e)
	{
		TRACE(DBG_LVL_DEBUG, "CZiBDb::UpdateNode - std::exception occured (%s):\n", typeid(e).name());
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, e.what());
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, "\n");
	}
	catch(...)
	{
		TRACE(DBG_LVL_DEBUG, "CZiBDb::UpdateNode - A system exception occured\n");
	}

	return bRet;
}

bool CZiBDb::FindNode(int64_t qwMacAddr, ZiBNodeRow *pNode)
{
	lock locker;
	bool bRet = false;

	TRACE(DBG_LVL_DBG4,"CZiBDb::FindNode\n");

	try
	{
		if(m_db->Connected())	
		{
			IBPP::Transaction tr1 = IBPP::TransactionFactory(m_db);
			tr1->Start();

			IBPP::Statement st1 = IBPP::StatementFactory(m_db, tr1);
			st1->Prepare("SELECT * FROM t_nodes WHERE mac_addr = ?");
			st1->Set(1, qwMacAddr);
			st1->Execute();

			int iCount = 0;
			while (st1->Fetch() ) // retrieve the rows
			{
				iCount++; 
				// mac_addr, nwk_addr, profile_id, device_id, online
				int64_t mac_addr = 0;
				int16_t nwk_addr=0, profile_id=0, device_id=0;
				bool online=0;

				if (pNode) 
				{
					st1->Get(1, pNode->GetMacAddrRef());
					st1->Get(2, pNode->GetNwkAddrRef());
					st1->Get(3, pNode->GetProfileIdRef());
					st1->Get(4, pNode->GetDeviceIdRef());
					st1->Get(5, pNode->GetOnlineRef());
					st1->Get(6, pNode->GetTagRef());
				}
			}

			tr1->CommitRetain();

			bRet = iCount>0? true: false;
		}
		else
		{
			TRACE(DBG_LVL_DEBUG,"Failure: Not connected to the database\n");
		}
	}
	catch(IBPP::Exception& e)
	{
		TRACE(DBG_LVL_DEBUG, "CZiBDb::FindNode - IBPP::Exception occured:\n");
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, e.what());
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, "\n");
		
		IBPP::SQLException* pe = dynamic_cast<IBPP::SQLException*>(&e);
		if (pe == 0)
			TRACE(DBG_LVL_DBG4, "CZiBDb::FindNode - Not an engine error\n");
		else 
			TRACE(DBG_LVL_DBG4, "CZiBDb::FindNode - Engine Code : %d\n", pe->EngineCode());

		if (pe->EngineCode() == 335544344) // todo: use a define
		{
			TRACE(DBG_LVL_DBG4,"CZiBDb::FindNode - THE FILE DOES NOT EXIST\n");
		}
	}
	catch(std::exception& e)
	{
		TRACE(DBG_LVL_DEBUG, "CZiBDb::FindNode - std::exception occured (%s):\n", typeid(e).name());
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, e.what());
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, "\n");
	}
	catch(...)
	{
		TRACE(DBG_LVL_DEBUG, "CZiBDb::FindNode - A system exception occured\n");
	}

	return bRet;
}

bool CZiBDb::DeleteNode(int64_t qwMacAddr)
{
	lock locker;
	bool bRet = false;

	TRACE(DBG_LVL_DBG4,"CZiBDb::DeleteNode\n");

	try
	{
		if(m_db->Connected())	
		{
			IBPP::Transaction tr1 = IBPP::TransactionFactory(m_db);
			tr1->Start();

			IBPP::Statement st1 = IBPP::StatementFactory(m_db, tr1);
			st1->Prepare("DELETE FROM t_nodes WHERE mac_addr = ?");
			st1->Set(1, qwMacAddr);
			st1->Execute();

			tr1->CommitRetain();

			bRet = true;
		}
		else
		{
			TRACE(DBG_LVL_DEBUG,"CZiBDb::DeleteNode - Failure: Not connected to the database\n");
		}
	}
	catch(IBPP::Exception& e)
	{
		TRACE(DBG_LVL_DEBUG, "CZiBDb::DeleteNode - IBPP::Exception occured:\n");
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, e.what());
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, "\n");
		
		IBPP::SQLException* pe = dynamic_cast<IBPP::SQLException*>(&e);
		if (pe == 0)
			TRACE(DBG_LVL_DBG4, "CZiBDb::DeleteNode - Not an engine error\n");
		else 
			TRACE(DBG_LVL_DBG4, "CZiBDb::DeleteNode - Engine Code : %d\n", pe->EngineCode());

		if (pe->EngineCode() == 335544344) // todo: use a define
		{
			TRACE(DBG_LVL_DBG4,"CZiBDb::DeleteNode - THE FILE DOES NOT EXIST\n");
		}
	}
	catch(std::exception& e)
	{
		TRACE(DBG_LVL_DEBUG, "CZiBDb::DeleteNode - std::exception occured (%s):\n", typeid(e).name());
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, e.what());
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, "\n");
	}
	catch(...)
	{
		TRACE(DBG_LVL_DEBUG, "CZiBDb::DeleteNode - A system exception occured\n");
	}

	return bRet;
}

bool CZiBDb::AllNodesOffline(void)
{
	lock locker;
	bool bRet = false;

	TRACE(DBG_LVL_DBG4, "CZiBDb::AllNodesOffline\n");

	try
	{
		if(m_db->Connected())	
		{
			IBPP::Transaction tr1 = IBPP::TransactionFactory(m_db);
			tr1->Start();

			IBPP::Statement st1 = IBPP::StatementFactory(m_db, tr1);
			st1->Prepare("UPDATE t_nodes SET online = 0");
			st1->Execute();
			tr1->CommitRetain();

			bRet = true;
		}
		else
		{
			TRACE(DBG_LVL_DBG4, "CZiBDb::AllNodesOffline - Failure: Not connected to the database\n");
		}
	}
	catch(IBPP::Exception& e)
	{
		TRACE(DBG_LVL_DEBUG, "CZiBDb::AllNodesOffline - IBPP::Exception occured:\n");
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, e.what());
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, "\n");
		
		IBPP::SQLException* pe = dynamic_cast<IBPP::SQLException*>(&e);
		if (pe == 0)
			TRACE(DBG_LVL_DBG4, "CZiBDb::AllNodesOffline - Not an engine error\n");
		else 
			TRACE(DBG_LVL_DBG4, "CZiBDb::AllNodesOffline - Engine Code : %d\n", pe->EngineCode());

		if (pe->EngineCode() == 335544344) // todo: use a define
		{
			TRACE(DBG_LVL_DBG4,"CZiBDb::AllNodesOffline - THE FILE DOES NOT EXIST\n");
		}
	}
	catch(std::exception& e)
	{
		TRACE(DBG_LVL_DEBUG, "CZiBDb::AllNodesOffline - std::exception occured (%s):\n", typeid(e).name());
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, e.what());
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, "\n");
	}
	catch(...)
	{
		TRACE(DBG_LVL_DEBUG, "CZiBDb::AllNodesOffline - A system exception occured\n");
	}

	return bRet;
}

// attributes **********************************************************
bool CZiBDb::InsertAttribute(ZiBAttributeRow &attrib)
{
	lock locker;

	return InsertAttribute(attrib.GetMacAddr(),
		attrib.GetEpNumber(),
		attrib.GetClusterId(),
		attrib.GetAttributeId(),
		attrib.GetValue());
}
bool CZiBDb::InsertAttribute(int64_t qwMacAddr,
	int16_t wEpNumber, 
	int16_t wClusterId, 
	int16_t wAttributeId, 
	std::string strValue)
{
	lock locker;
	bool bRet = false;

	TRACE(DBG_LVL_DBG4,"CZiBDb::InsertAttribute\n");

	try
	{
		if(m_db->Connected())	
		{
			IBPP::Transaction tr1 = IBPP::TransactionFactory(m_db);
			tr1->Start();


			IBPP::Statement st1 = IBPP::StatementFactory(m_db, tr1);
			st1->Prepare(	"INSERT INTO t_attributes (mac_addr, ep_number, cluster_id, attribute_id, attribute_value)"
							"VALUES (?, ?, ?, ?, ?);");
			st1->Set(1, qwMacAddr);
			st1->Set(2, wEpNumber);
			st1->Set(3, wClusterId);
			st1->Set(4, wAttributeId);
			st1->Set(5, strValue);
			st1->Execute();

			tr1->CommitRetain();

			bRet = true;
		}
		else
		{
			TRACE(DBG_LVL_DEBUG,"CZiBDb::InsertAttribute - Failure: Not connected to the database\n");
		}
	}
	catch(IBPP::Exception& e)
	{
		TRACE(DBG_LVL_DEBUG, "CZiBDb::InsertAttribute - IBPP::Exception occured:\n");
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, e.what());
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, "\n");
		
		IBPP::SQLException* pe = dynamic_cast<IBPP::SQLException*>(&e);
		if (pe == 0)
			TRACE(DBG_LVL_DBG4, "CZiBDb::InsertAttribute - Not an engine error\n");
		else 
			TRACE(DBG_LVL_DBG4, "CZiBDb::InsertAttribute - Engine Code : %d\n", pe->EngineCode());

		if (pe->EngineCode() == 335544344) // todo: use a define
		{
			TRACE(DBG_LVL_DBG4,"CZiBDb::InsertAttribute - THE FILE DOES NOT EXIST\n");
		}
	}
	catch(std::exception& e)
	{
		TRACE(DBG_LVL_DEBUG, "CZiBDb::InsertAttribute - std::exception occured (%s):\n", typeid(e).name());
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, e.what());
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, "\n");
	}
	catch(...)
	{
		TRACE(DBG_LVL_DEBUG, "CZiBDb::InsertAttribute - A system exception occured\n");
	}

	return bRet;
}

bool CZiBDb::UpdateAttribute(ZiBAttributeRow &attrib)
{
	lock locker;

	return UpdateAttribute(attrib.GetMacAddr(),
		attrib.GetEpNumber(),
		attrib.GetClusterId(),
		attrib.GetAttributeId(),
		attrib.GetValue());
}

bool CZiBDb::UpdateAttribute(int64_t qwMacAddr,
	int16_t wEpNumber, 
	int16_t wClusterId, 
	int16_t wAttributeId,
	const string &strValue)
{
	lock locker;
	bool bRet = false;

	TRACE(DBG_LVL_DBG4,"CZiBDb::UpdateAttribute\n");

	try
	{
		if(m_db->Connected())	
		{
			IBPP::Transaction tr1 = IBPP::TransactionFactory(m_db);
			tr1->Start();

			IBPP::Statement st1 = IBPP::StatementFactory(m_db, tr1);
			st1->Prepare(	"UPDATE t_attributes "
							"SET attribute_value = ? "
							"WHERE mac_addr = ? "
							"AND ep_number = ? "
							"AND cluster_id = ? "
							"AND attribute_id = ?; " 
						);
			st1->Set(1, strValue);
			st1->Set(2, qwMacAddr);
			st1->Set(3, wEpNumber);
			st1->Set(4, wClusterId);
			st1->Set(5, wAttributeId);
			st1->Execute();

			tr1->CommitRetain();

			bRet = true;
		}
		else
		{
			TRACE(DBG_LVL_DEBUG,"CZiBDb::UpdateAttribute - Failure: Not connected to the database\n");
		}
	}
	catch(IBPP::Exception& e)
	{
		TRACE(DBG_LVL_DEBUG, "CZiBDb::UpdateAttribute - IBPP::Exception occured:\n");
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, e.what());
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, "\n");
		
		IBPP::SQLException* pe = dynamic_cast<IBPP::SQLException*>(&e);
		if (pe == 0)
			TRACE(DBG_LVL_DBG4, "CZiBDb::UpdateAttribute - Not an engine error\n");
		else 
			TRACE(DBG_LVL_DBG4, "CZiBDb::UpdateAttribute - Engine Code : %d\n", pe->EngineCode());

		if (pe->EngineCode() == 335544344) // todo: use a define
		{
			TRACE(DBG_LVL_DBG4,"CZiBDb::UpdateAttribute - THE FILE DOES NOT EXIST\n");
		}
	}
	catch(std::exception& e)
	{
		TRACE(DBG_LVL_DEBUG, "CZiBDb::UpdateAttribute - std::exception occured (%s):\n", typeid(e).name());
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, e.what());
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, "\n");
	}
	catch(...)
	{
		TRACE(DBG_LVL_DEBUG, "CZiBDb::UpdateAttribute - A system exception occured\n");
	}

	return bRet;
}

bool CZiBDb::FindAttribute(ZiBAttributeRow &attrib)
{
	lock locker;

	return FindAttribute(attrib.GetMacAddr(),
		attrib.GetEpNumber(),
		attrib.GetClusterId(),
		attrib.GetAttributeId(),
		attrib.GetValueRef());
}

bool CZiBDb::FindAttribute(int64_t qwMacAddr,
	int16_t wEpNumber, 
	int16_t wClusterId, 
	int16_t wAttributeId,
	string& strAttributeValueReturn)
{
	lock locker;
	bool bRet = false;

	TRACE(DBG_LVL_DBG4,"CZiBDb::FindAttribute\n");

	try
	{
		IBPP::Transaction tr1 = IBPP::TransactionFactory(m_db);
		tr1->Start();

		IBPP::Statement st1 = IBPP::StatementFactory(m_db, tr1);
		st1->Prepare("SELECT * FROM t_attributes WHERE "
			"mac_addr = ? AND "
			"ep_number = ? AND "
			"cluster_id = ? AND "
			"attribute_id = ?;");

		st1->Set(1, qwMacAddr);
		st1->Set(2, wEpNumber);
		st1->Set(3, wClusterId);
		st1->Set(4, wAttributeId);

		st1->Execute();

		int iCount = 0;
		while (st1->Fetch() ) // retrieve the rows
		{
			iCount++; 
			// mac_addr, nwk_addr, profile_id, device_id, online
			int64_t mac_addr=0;
			int16_t ep_number=0, cluster_id=0, attribute_id=0;
			std::string attribute_value="";

			st1->Get("attribute_value", strAttributeValueReturn);
		}

		tr1->CommitRetain();

		bRet = iCount>0? true: false;
	}
	catch(IBPP::Exception& e)
	{
		TRACE(DBG_LVL_DEBUG, "CZiBDb::FindAttribute - IBPP::Exception occured:\n");
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, e.what());
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, "\n");
		
		IBPP::SQLException* pe = dynamic_cast<IBPP::SQLException*>(&e);
		if (pe == 0)
			TRACE(DBG_LVL_DBG4, "CZiBDb::FindAttribute - Not an engine error\n");
		else 
			TRACE(DBG_LVL_DBG4, "CZiBDb::FindAttribute - Engine Code : %d\n", pe->EngineCode());

		if (pe->EngineCode() == 335544344) // todo: use a define
		{
			TRACE(DBG_LVL_DBG4,"CZiBDb::FindAttribute - THE FILE DOES NOT EXIST\n");
		}
	}
	catch(std::exception& e)
	{
		TRACE(DBG_LVL_DEBUG, "CZiBDb::FindAttribute - std::exception occured (%s):\n", typeid(e).name());
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, e.what());
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, "\n");
	}
	catch(...)
	{
		TRACE(DBG_LVL_DEBUG, "CZiBDb::FindAttribute - A system exception occured\n");
	}

	return bRet;
}

//bool CZiBDb::DeleteAttribute(int64_t qwMacAddr)
//{
//	TRACE(DBG_LVL_DBG4,"CZiBDb::DeleteAttribute\n");
//
//	IBPP::Transaction tr1 = IBPP::TransactionFactory(m_db);
//	tr1->Start();
//
//	IBPP::Statement st1 = IBPP::StatementFactory(m_db, tr1);
//	st1->Prepare("DELETE FROM t_attributes WHERE mac_addr = ?");
//	st1->Set(1, qwMacAddr);
//	st1->Execute();
//
//	tr1->CommitRetain();
//
//	return true;
//}

bool CZiBDb::GetAttributeAndLimits(int64_t qwMacAddr,
	int16_t wEpNumber, 
	int16_t wClusterId, 
	int16_t wAttributeId,
	string &strAttributeValueReturn,
	string &strUpLimitReturn,
	string &strDownLimitReturn,
	DWORD &dwLimitAction)
{
	lock locker;
	bool bRet = false;

	TRACE(DBG_LVL_DBG4,"CZiBDb::GetAttributeAndLimits\n");

	try
	{
		IBPP::Transaction tr1 = IBPP::TransactionFactory(m_db);
		tr1->Start();

		IBPP::Statement st1 = IBPP::StatementFactory(m_db, tr1);
		st1->Prepare("SELECT * FROM t_attributes WHERE "
			"mac_addr = ? AND "
			"ep_number = ? AND "
			"cluster_id = ? AND "
			"attribute_id = ?;");

		st1->Set(1, qwMacAddr);
		st1->Set(2, wEpNumber);
		st1->Set(3, wClusterId);
		st1->Set(4, wAttributeId);

		st1->Execute();

		int iCount = 0;
		while (st1->Fetch() ) // retrieve the rows
		{
			iCount++; 

			st1->Get("attribute_value", strAttributeValueReturn);
			st1->Get("up_limit", strUpLimitReturn);
			st1->Get("down_limit", strDownLimitReturn);
			st1->Get("limit_action", (int32_t&)dwLimitAction); 
		}

		tr1->CommitRetain();

		bRet = iCount>0? true: false;
	}
	catch(IBPP::Exception& e)
	{
		TRACE(DBG_LVL_DEBUG, "CZiBDb::GetAttributeAndLimits - IBPP::Exception occured:\n");
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, e.what());
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, "\n");
		
		IBPP::SQLException* pe = dynamic_cast<IBPP::SQLException*>(&e);
		if (pe == 0)
			TRACE(DBG_LVL_DBG4, "CZiBDb::GetAttributeAndLimits - Not an engine error\n");
		else 
			TRACE(DBG_LVL_DBG4, "CZiBDb::GetAttributeAndLimits - Engine Code : %d\n", pe->EngineCode());

		if (pe->EngineCode() == 335544344) // todo: use a define
		{
			TRACE(DBG_LVL_DBG4,"CZiBDb::GetAttributeAndLimits - THE FILE DOES NOT EXIST\n");
		}
	}
	catch(std::exception& e)
	{
		TRACE(DBG_LVL_DEBUG, "CZiBDb::GetAttributeAndLimits - std::exception occured (%s):\n", typeid(e).name());
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, e.what());
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, "\n");
	}
	catch(...)
	{
		TRACE(DBG_LVL_DEBUG, "CZiBDb::GetAttributeAndLimits - A system exception occured\n");
	}

	return bRet;
}




// attributes history **********************************************************
bool CZiBDb::InsertAttributeHistory(ZiBAttributeHistoryRow &attrib)
{
	lock locker;

	return InsertAttributeHistory(attrib.GetMacAddr(),
		attrib.GetEpNumber(),
		attrib.GetClusterId(),
		attrib.GetAttributeId(),
		attrib.GetTimeStamp(),
		attrib.GetValue());
}
bool CZiBDb::InsertAttributeHistory(int64_t qwMacAddr,
	int16_t wEpNumber,
	int16_t wClusterId,
	int16_t wAttributeId,
	const IBPP::Timestamp &timeMeasure,
	const std::string &strValue)
{
	lock locker;
	bool bRet = false;

	TRACE(DBG_LVL_DBG4,"CZiBDb::InsertAttributeHistory\n");

	try
	{
		if(m_db->Connected())	
		{
			double fValue = atof(strValue.c_str());

			IBPP::Transaction tr1 = IBPP::TransactionFactory(m_db);
			tr1->Start();


			IBPP::Statement st1 = IBPP::StatementFactory(m_db, tr1);
			st1->Prepare(	"INSERT INTO t_attributes_history (mac_addr, ep_number, cluster_id, attribute_id, measure_time, attribute_value, attribute_value_double)"
							"VALUES (?, ?, ?, ?, ?, ?, ?);" );
			st1->Set(1, qwMacAddr);
			st1->Set(2, wEpNumber);
			st1->Set(3, wClusterId);
			st1->Set(4, wAttributeId);
			st1->Set(5, timeMeasure);
			st1->Set(6, strValue);
			st1->Set(7, fValue);
			st1->Execute();

			tr1->CommitRetain();

			bRet = true;
		}
		else
		{
			TRACE(DBG_LVL_DEBUG,"CZiBDb::InsertAttributeHistory - Failure: Not connected to the database\n");
		}
	}
	catch(IBPP::Exception& e)
	{
		TRACE(DBG_LVL_DEBUG, "CZiBDb::InsertAttributeHistory - IBPP::Exception occured:\n");
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, e.what());
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, "\n");
		
		IBPP::SQLException* pe = dynamic_cast<IBPP::SQLException*>(&e);
		if (pe == 0)
			TRACE(DBG_LVL_DBG4, "CZiBDb::InsertAttributeHistory - Not an engine error\n");
		else 
			TRACE(DBG_LVL_DBG4, "CZiBDb::InsertAttributeHistory - Engine Code : %d\n", pe->EngineCode());

		if (pe->EngineCode() == 335544344) // todo: use a define
		{
			TRACE(DBG_LVL_DBG4,"CZiBDb::InsertAttributeHistory - THE FILE DOES NOT EXIST\n");
		}
	}
	catch(std::exception& e)
	{
		TRACE(DBG_LVL_DEBUG, "CZiBDb::InsertAttributeHistory - std::exception occured (%s):\n", typeid(e).name());
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, e.what());
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, "\n");
	}
	catch(...)
	{
		TRACE(DBG_LVL_DEBUG, "CZiBDb::InsertAttributeHistory - A system exception occured\n");
	}

	return bRet;
}

bool CZiBDb::UpdateAttributeHistory(ZiBAttributeHistoryRow &attrib)
{
	lock locker;

	return UpdateAttributeHistory(attrib.GetMacAddr(),
		attrib.GetEpNumber(),
		attrib.GetClusterId(),
		attrib.GetAttributeId(),
		attrib.GetTimeStamp(),
		attrib.GetValue());
}

bool CZiBDb::UpdateAttributeHistory(int64_t qwMacAddr,
	int16_t wEpNumber, 
	int16_t wClusterId, 
	int16_t wAttributeId,
	const IBPP::Timestamp &timeMeasure,
	const string &strValue)
{
	lock locker;
	bool bRet = false;

	TRACE(DBG_LVL_DBG4,"CZiBDb::UpdateAttributeHistory\n");

	try
	{
		if(m_db->Connected())	
		{
			double fValue = atof(strValue.c_str());

			IBPP::Transaction tr1 = IBPP::TransactionFactory(m_db);
			tr1->Start();

			IBPP::Statement st1 = IBPP::StatementFactory(m_db, tr1);
			st1->Prepare(	"UPDATE t_attributes_history "
							"SET attribute_value = ?, attribute_value_double = ? "
							"WHERE mac_addr = ? "
							"AND ep_number = ? "
							"AND cluster_id = ? "
							"AND attribute_id = ? " 
							"AND measure_time = ?; " );
			st1->Set(1, strValue);
			st1->Set(2, fValue);
			st1->Set(3, qwMacAddr);
			st1->Set(4, wEpNumber);
			st1->Set(5, wClusterId);
			st1->Set(6, wAttributeId);
			st1->Set(7, timeMeasure);
			st1->Execute();

			tr1->CommitRetain();

			bRet = true;
		}
		else
		{
			TRACE(DBG_LVL_DEBUG,"CZiBDb::UpdateAttributeHistory - Failure: Not connected to the database\n");
		}
	}
	catch(IBPP::Exception& e)
	{
		TRACE(DBG_LVL_DEBUG, "CZiBDb::UpdateAttributeHistory - IBPP::Exception occured:\n");
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, e.what());
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, "\n");
		
		IBPP::SQLException* pe = dynamic_cast<IBPP::SQLException*>(&e);
		if (pe == 0)
			TRACE(DBG_LVL_DBG4, "CZiBDb::UpdateAttributeHistory - Not an engine error\n");
		else 
			TRACE(DBG_LVL_DBG4, "CZiBDb::UpdateAttributeHistory - Engine Code : %d\n", pe->EngineCode());

		if (pe->EngineCode() == 335544344) // todo: use a define
		{
			TRACE(DBG_LVL_DBG4,"CZiBDb::UpdateAttributeHistory - THE FILE DOES NOT EXIST\n");
		}
	}
	catch(std::exception& e)
	{
		TRACE(DBG_LVL_DEBUG, "CZiBDb::UpdateAttributeHistory - std::exception occured (%s):\n", typeid(e).name());
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, e.what());
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, "\n");
	}
	catch(...)
	{
		TRACE(DBG_LVL_DEBUG, "CZiBDb::UpdateAttributeHistory - A system exception occured\n");
	}

	return bRet;
}

bool CZiBDb::FindAttributeHistory(ZiBAttributeHistoryRow &attrib)
{
	lock locker;

	return FindAttributeHistory(attrib.GetMacAddr(),
		attrib.GetEpNumber(),
		attrib.GetClusterId(),
		attrib.GetAttributeId(),
		attrib.GetTimeStamp(),
		attrib.GetValueRef());
}

bool CZiBDb::FindAttributeHistory(int64_t qwMacAddr,
	int16_t wEpNumber, 
	int16_t wClusterId, 
	int16_t wAttributeId,
	const IBPP::Timestamp &timeMeasure,
	string& strAttributeValueReturn)
{
	lock locker;
	bool bRet = false;

	TRACE(DBG_LVL_DBG4,"CZiBDb::FindAttributeHistory\n");

	try
	{
		IBPP::Transaction tr1 = IBPP::TransactionFactory(m_db);
		tr1->Start();

		IBPP::Statement st1 = IBPP::StatementFactory(m_db, tr1);
		st1->Prepare("SELECT * FROM t_attributes_history WHERE "
			"mac_addr = ? AND "
			"ep_number = ? AND "
			"cluster_id = ? AND "
			"attribute_id = ? AND "
			"measure_time = ?;");

		st1->Set(1, qwMacAddr);
		st1->Set(2, wEpNumber);
		st1->Set(3, wClusterId);
		st1->Set(4, wAttributeId);
		st1->Set(5, timeMeasure);

		st1->Execute();

		int iCount = 0;
		while (st1->Fetch() ) // retrieve the rows
		{
			iCount++; 
			st1->Get("attribute_value", strAttributeValueReturn);
		}

		tr1->CommitRetain();

		bRet = iCount>0? true: false;
	}
	catch(IBPP::Exception& e)
	{
		TRACE(DBG_LVL_DEBUG, "CZiBDb::FindAttributeHistory - IBPP::Exception occured:\n");
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, e.what());
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, "\n");
		
		IBPP::SQLException* pe = dynamic_cast<IBPP::SQLException*>(&e);
		if (pe == 0)
			TRACE(DBG_LVL_DBG4, "CZiBDb::FindAttributeHistory - Not an engine error\n");
		else 
			TRACE(DBG_LVL_DBG4, "CZiBDb::FindAttributeHistory - Engine Code : %d\n", pe->EngineCode());

		if (pe->EngineCode() == 335544344) // todo: use a define
		{
			TRACE(DBG_LVL_DBG4,"CZiBDb::FindAttributeHistory - THE FILE DOES NOT EXIST\n");
		}
	}
	catch(std::exception& e)
	{
		TRACE(DBG_LVL_DEBUG, "CZiBDb::FindAttributeHistory - std::exception occured (%s):\n", typeid(e).name());
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, e.what());
		TRACE(DBG_LVL_DBG4 | DBG_MASK_NOHEADER, "\n");
	}
	catch(...)
	{
		TRACE(DBG_LVL_DEBUG, "CZiBDb::FindAttributeHistory - A system exception occured\n");
	}

	return bRet;
}

//bool CZiBDb::DeleteAttributeHistory(int64_t qwMacAddr)
//{
//	TRACE(DBG_LVL_DBG4,"CZiBDb::DeleteAttribute\n");
//
//	IBPP::Transaction tr1 = IBPP::TransactionFactory(m_db);
//	tr1->Start();
//
//	IBPP::Statement st1 = IBPP::StatementFactory(m_db, tr1);
//	st1->Prepare("DELETE FROM t_attributes WHERE mac_addr = ?");
//	st1->Set(1, qwMacAddr);
//	st1->Execute();
//
//	tr1->CommitRetain();
//
//	return true;
//}