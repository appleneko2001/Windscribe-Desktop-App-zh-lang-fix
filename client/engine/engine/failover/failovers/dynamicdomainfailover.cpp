#include "dynamicdomainfailover.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QUrlQuery>

#include "engine/networkaccessmanager/networkaccessmanager.h"
#include "engine/dnsresolver/dnsserversconfiguration.h"
#include "utils/ws_assert.h"
#include "utils/utils.h"

namespace failover {

void DynamicDomainFailover::getData(bool bIgnoreSslErrors)
{
    QUrl url(urlString_);
    QUrlQuery query;
    query.addQueryItem("name", domainName_);
    query.addQueryItem("type", "TXT");
    url.setQuery(query);

    NetworkRequest networkRequest(url, 5000, true, DnsServersConfiguration::instance().getCurrentDnsServers(), bIgnoreSslErrors);
    networkRequest.setContentTypeHeader("accept: application/dns-json");
    NetworkReply *reply = networkAccessManager_->get(networkRequest);
    connect(reply, &NetworkReply::finished, [=]() {
        if (!reply->isSuccess()) {
            emit finished(QVector<FailoverData>());
        } else {
            QString hostname = parseHostnameFromJson(reply->readAll());
            if (!hostname.isEmpty())
                emit finished(QVector<FailoverData>() << FailoverData(hostname));
            else
                emit finished(QVector<FailoverData>());
        }
        reply->deleteLater();
    });
}

QString DynamicDomainFailover::name() const
{
    QUrl url(urlString_);
    // the domain name has been reduced to 9 characters for log security
    return "dyn: " + url.host().left(3);
}

// returns an empty string if parsing failed
QString DynamicDomainFailover::parseHostnameFromJson(const QByteArray &arr)
{
    QJsonParseError errCode;
    QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
    if (errCode.error != QJsonParseError::NoError || !doc.isObject())
        return QString();

    QJsonObject jsonObject = doc.object();
    if (!jsonObject.contains("Status") || jsonObject["Status"].toInt(1) != 0)
        return QString();

    if (!jsonObject.contains("Answer") || !jsonObject["Answer"].isArray())
        return QString();

    QJsonArray jsonArray = jsonObject["Answer"].toArray();
    WS_ASSERT(jsonArray.size() == 1);
    if (jsonArray.isEmpty())
        return QString();

    QJsonObject jsonAnswer = jsonArray[0].toObject();
    if (!jsonAnswer.contains("data"))
        return QString();

    return jsonAnswer["data"].toString().remove("\"");  // remove quotes
}

} // namespace failover

