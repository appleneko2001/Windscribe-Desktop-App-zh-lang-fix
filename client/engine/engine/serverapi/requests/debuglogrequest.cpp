#include "debuglogrequest.h"

#include <QJsonDocument>
#include <QJsonObject>

#include "utils/logger.h"
#include "engine/utils/urlquery_utils.h"

namespace server_api {

DebugLogRequest::DebugLogRequest(QObject *parent, const QString &username, const QString &strLog) :
    BaseRequest(parent, RequestType::kPost),
    username_(username),
    strLog_(strLog)
{
}

QString DebugLogRequest::contentTypeHeader() const
{
    return "Content-type: application/x-www-form-urlencoded";
}

QByteArray DebugLogRequest::postData() const
{
    QUrlQuery postData;
    QByteArray ba = strLog_.toUtf8();
    postData.addQueryItem("logfile", ba.toBase64());
    if (!username_.isEmpty())
        postData.addQueryItem("username", username_);
    urlquery_utils::addAuthQueryItems(postData);
    urlquery_utils::addPlatformQueryItems(postData);
    return postData.toString(QUrl::FullyEncoded).toUtf8();
}

QUrl DebugLogRequest::url(const QString &domain) const
{
    return  QUrl("https://" + hostname(domain, SudomainType::kApi) + "/Report/applog");
}

QString DebugLogRequest::name() const
{
    return "DebugLog";
}

void DebugLogRequest::handle(const QByteArray &arr)
{
    QJsonParseError errCode;
    QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
    if (errCode.error != QJsonParseError::NoError || !doc.isObject()) {
        qCDebugMultiline(LOG_SERVER_API) << arr;
        qCDebug(LOG_SERVER_API) << "Failed parse JSON for Report";
        setNetworkRetCode(SERVER_RETURN_INCORRECT_JSON);
        return;
    }

    QJsonObject jsonObject = doc.object();
    if (!jsonObject.contains("data")) {
        qCDebugMultiline(LOG_SERVER_API) << arr;
        qCDebug(LOG_SERVER_API) << "Failed parse JSON for Report";
        setNetworkRetCode(SERVER_RETURN_INCORRECT_JSON);
        return;
    }
    QJsonObject jsonData =  jsonObject["data"].toObject();
    if (!jsonData.contains("success")) {
        qCDebugMultiline(LOG_SERVER_API) << arr;
        qCDebug(LOG_SERVER_API) << "Failed parse JSON for Report";
        setNetworkRetCode(SERVER_RETURN_INCORRECT_JSON);
        return;
    }

    if (jsonData["success"].toInt() != 1) {
        setNetworkRetCode(SERVER_RETURN_INCORRECT_JSON);
        return;
    }
}

} // namespace server_api {
