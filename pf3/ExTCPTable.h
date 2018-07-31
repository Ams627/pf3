#pragma once

class ExtTcpTable
{
    void *pTable;
public:
    ExtTcpTable()
    {
        pTable = malloc(sizeof MIB_TCP6TABLE_OWNER_PID);
        DWORD size = sizeof MIB_TCP6TABLE_OWNER_PID;
        DWORD result = GetExtendedTcpTable(pTable, &size,
                                           TRUE, AF_INET6, TCP_TABLE_OWNER_PID_LISTENER, 0);
        assert(pTable != NULL);

        if (result == ERROR_INSUFFICIENT_BUFFER)
        {
            pTable = realloc(pTable, size);
            assert(pTable != NULL);
            result = GetExtendedTcpTable(pTable, &size,
                                         TRUE, AF_INET6, TCP_TABLE_OWNER_PID_LISTENER, 0);
            assert(result != ERROR_INSUFFICIENT_BUFFER);
        }
    }

    size_t GetCount()
    {
        MIB_TCP6TABLE_OWNER_PID *pMIB = static_cast<MIB_TCP6TABLE_OWNER_PID*>(pTable);
        return pMIB->dwNumEntries;
    }

    MIB_TCP6ROW_OWNER_PID GetEntry(size_t n)
    {
        assert(n < GetCount());
        MIB_TCP6TABLE_OWNER_PID *pMIB = static_cast<MIB_TCP6TABLE_OWNER_PID*>(pTable);
        return pMIB->table[n];
    }

    virtual ~ExtTcpTable()
    {
        free(pTable);
    }
};
