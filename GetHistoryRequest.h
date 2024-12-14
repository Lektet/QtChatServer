#ifndef GETHISTORYREQUEST_H
#define GETHISTORYREQUEST_H

#include <ChatDataRequest.h>

class GetHistoryRequest : public ChatDataRequest
{
public:
    GetHistoryRequest(int actionId);
};

#endif // GETHISTORYREQUEST_H
