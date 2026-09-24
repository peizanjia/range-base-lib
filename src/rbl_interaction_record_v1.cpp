#include "rbl_interaction_record_v1.h"

#include <QJsonDocument>
#include <QJsonParseError>
#include <QRegularExpression>
#include <QSet>

#include <cmath>
#include <utility>

namespace
{
const QRegularExpression uuidPattern(
    QStringLiteral("^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$"));
const QRegularExpression timestampPattern(
    QStringLiteral("^[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}\\.[0-9]{3}Z$"));
const QRegularExpression tokenPattern(QStringLiteral("^[A-Za-z][A-Za-z0-9._:/-]*$"));
const QRegularExpression versionPattern(QStringLiteral("^[0-9]+\\.[0-9]+\\.[0-9]+$"));
const QRegularExpression hashPattern(QStringLiteral("^[0-9a-f]{64}$"));

bool fail(const QString &message, QString *error)
{
    if (error) *error = message;
    return false;
}

QString uuidText(const QUuid &id)
{
    return id.toString(QUuid::WithoutBraces);
}

bool validUuid(const QUuid &id)
{
    return !id.isNull() && uuidPattern.match(uuidText(id)).hasMatch();
}

bool validToken(const QString &value, int limit = 128)
{
    return !value.isEmpty() && value.size() <= limit && tokenPattern.match(value).hasMatch();
}

bool validVersion(const QString &value)
{
    return value.size() <= 64 && versionPattern.match(value).hasMatch();
}

bool validHash(const QString &value)
{
    return value.isEmpty() || hashPattern.match(value).hasMatch();
}

bool validPayload(const QJsonValue &value, int depth = 0)
{
    if (depth > 16 || value.isUndefined() || value.isNull()) return false;
    if (value.isDouble()) return std::isfinite(value.toDouble());
    if (value.isString()) return value.toString().size() <= 4096;
    if (value.isArray())
    {
        const QJsonArray array = value.toArray();
        if (array.size() > 1024) return false;
        for (const QJsonValue &item : array)
            if (!validPayload(item,depth+1)) return false;
    }
    if (value.isObject())
    {
        const QJsonObject object = value.toObject();
        if (object.size() > 1024) return false;
        for (auto it=object.constBegin();it!=object.constEnd();++it)
            if (it.key().size() > 256 || !validPayload(it.value(),depth+1)) return false;
    }
    return true;
}

bool exactKeys(const QJsonObject &object, const QSet<QString> &expected)
{
    if (object.size() != expected.size()) return false;
    for (auto it=object.constBegin();it!=object.constEnd();++it)
        if (!expected.contains(it.key())) return false;
    return true;
}

bool readUuid(const QJsonObject &object, const QString &key, QUuid &id, bool nullable)
{
    const QJsonValue value = object.value(key);
    if (nullable && value.isNull()) { id = QUuid(); return true; }
    if (!value.isString() || !uuidPattern.match(value.toString()).hasMatch()) return false;
    id = QUuid(value.toString());
    return validUuid(id);
}

QJsonValue nullableUuid(const QUuid &id)
{
    return id.isNull() ? QJsonValue(QJsonValue::Null) : QJsonValue(uuidText(id));
}

QJsonValue nullableHash(const QString &hash)
{
    return hash.isEmpty() ? QJsonValue(QJsonValue::Null) : QJsonValue(hash);
}
}

bool RInteractionRecordV1::validate(QString *error) const
{
    if (!validUuid(recordId) || (!parentRecordId.isNull() && !validUuid(parentRecordId)) ||
        (!parentRecordId.isNull() && parentRecordId == recordId) ||
        !validUuid(sessionId) || !validUuid(projectId) || !validUuid(modelId) ||
        (!runId.isNull() && !validUuid(runId)))
        return fail(QStringLiteral("Invalid interaction UUID or self-parent"),error);
    if (!timestampUtc.isValid() || timestampUtc.timeSpec() != Qt::UTC ||
        !timestampPattern.match(timestampUtc.toString(Qt::ISODateWithMs)).hasMatch())
        return fail(QStringLiteral("Interaction timestamp must be UTC with milliseconds"),error);
    static const QSet<QString> actors = {QStringLiteral("user"),QStringLiteral("agent"),
        QStringLiteral("solver"),QStringLiteral("controller"),QStringLiteral("system"),
        QStringLiteral("plugin")};
    if (!actors.contains(actorType) || actorId.isEmpty() || actorId.size() > 128 ||
        actorId.contains(QLatin1Char('\n')) || actorId.contains(QLatin1Char('\r')) ||
        !validToken(modelRevision) || !validToken(workflowStage) ||
        !validToken(commandId) || !validVersion(commandVersion) ||
        !validVersion(applicationVersion))
        return fail(QStringLiteral("Invalid actor, workflow, command or version"),error);
    static const QSet<QString> approvals = {QStringLiteral("not_required"),
        QStringLiteral("pending"),QStringLiteral("approved"),QStringLiteral("rejected"),
        QStringLiteral("modified")};
    if (!approvals.contains(approvalState) || !validToken(resultStatus) ||
        !validHash(preStateHash) || !validHash(postStateHash))
        return fail(QStringLiteral("Invalid approval, status or state hash"),error);
    if (!validPayload(parameters) || !validPayload(selectionSnapshot) ||
        !(agentProposal.isNull() || (agentProposal.isObject() && validPayload(agentProposal))))
        return fail(QStringLiteral("Invalid parameters, selection or agent proposal"),error);
    if (diagnostics.size() > 1024 || artifactRefs.size() > 1024 ||
        pluginVersions.size() > 1024) return fail(QStringLiteral("Interaction list exceeds limit"),error);
    for (const QJsonValue &value : diagnostics)
    {
        if (!value.isObject()) return fail(QStringLiteral("Invalid diagnostic"),error);
        const QJsonObject item = value.toObject();
        if (!exactKeys(item,{QStringLiteral("code"),QStringLiteral("severity"),QStringLiteral("message")}) ||
            !validToken(item.value(QStringLiteral("code")).toString()) ||
            !QSet<QString>{QStringLiteral("info"),QStringLiteral("warning"),QStringLiteral("error")}
                .contains(item.value(QStringLiteral("severity")).toString()) ||
            !item.value(QStringLiteral("message")).isString() ||
            item.value(QStringLiteral("message")).toString().isEmpty() ||
            item.value(QStringLiteral("message")).toString().size() > 4096)
            return fail(QStringLiteral("Invalid diagnostic fields"),error);
    }
    for (const QJsonValue &value : artifactRefs)
    {
        if (!value.isObject()) return fail(QStringLiteral("Invalid artifact reference"),error);
        const QJsonObject item = value.toObject();
        if (!exactKeys(item,{QStringLiteral("artifactId"),QStringLiteral("sha256")}) ||
            !validToken(item.value(QStringLiteral("artifactId")).toString()) ||
            !item.value(QStringLiteral("sha256")).isString() ||
            !hashPattern.match(item.value(QStringLiteral("sha256")).toString()).hasMatch())
            return fail(QStringLiteral("Invalid artifact reference fields"),error);
    }
    for (auto it=pluginVersions.constBegin();it!=pluginVersions.constEnd();++it)
        if (!validToken(it.key()) || !it.value().isString() ||
            !validVersion(it.value().toString()))
            return fail(QStringLiteral("Invalid plugin version"),error);
    return true;
}

bool RInteractionRecordV1::toJson(QByteArray &bytes, QString *error) const
{
    if (!validate(error)) return false;
    const QJsonObject object{
        {QStringLiteral("schemaVersion"),SchemaVersion},
        {QStringLiteral("recordId"),uuidText(recordId)},
        {QStringLiteral("parentRecordId"),nullableUuid(parentRecordId)},
        {QStringLiteral("timestamp"),timestampUtc.toString(Qt::ISODateWithMs)},
        {QStringLiteral("actorType"),actorType}, {QStringLiteral("actorId"),actorId},
        {QStringLiteral("sessionId"),uuidText(sessionId)},
        {QStringLiteral("projectId"),uuidText(projectId)},
        {QStringLiteral("modelId"),uuidText(modelId)},
        {QStringLiteral("modelRevision"),modelRevision},
        {QStringLiteral("runId"),nullableUuid(runId)},
        {QStringLiteral("workflowStage"),workflowStage},
        {QStringLiteral("commandId"),commandId},
        {QStringLiteral("commandVersion"),commandVersion},
        {QStringLiteral("parameters"),parameters},
        {QStringLiteral("selectionSnapshot"),selectionSnapshot},
        {QStringLiteral("agentProposal"),agentProposal},
        {QStringLiteral("approvalState"),approvalState},
        {QStringLiteral("preStateHash"),nullableHash(preStateHash)},
        {QStringLiteral("postStateHash"),nullableHash(postStateHash)},
        {QStringLiteral("resultStatus"),resultStatus},
        {QStringLiteral("diagnostics"),diagnostics},
        {QStringLiteral("artifactRefs"),artifactRefs},
        {QStringLiteral("applicationVersion"),applicationVersion},
        {QStringLiteral("pluginVersions"),pluginVersions}};
    const QByteArray candidate = QJsonDocument(object).toJson(QJsonDocument::Compact);
    if (candidate.size() > MaxJsonBytes)
        return fail(QStringLiteral("Interaction JSON exceeds size limit"),error);
    bytes = candidate;
    return true;
}

bool RInteractionRecordV1::fromJson(const QByteArray &bytes,
                                    RInteractionRecordV1 &record, QString *error)
{
    if (bytes.isEmpty() || bytes.size() > MaxJsonBytes)
        return fail(QStringLiteral("Interaction JSON size is invalid"),error);
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(bytes,&parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject())
        return fail(QStringLiteral("Interaction JSON is not an object"),error);
    const QJsonObject object = document.object();
    static const QSet<QString> keys = {QStringLiteral("schemaVersion"),
        QStringLiteral("recordId"),QStringLiteral("parentRecordId"),QStringLiteral("timestamp"),
        QStringLiteral("actorType"),QStringLiteral("actorId"),QStringLiteral("sessionId"),
        QStringLiteral("projectId"),QStringLiteral("modelId"),QStringLiteral("modelRevision"),
        QStringLiteral("runId"),QStringLiteral("workflowStage"),QStringLiteral("commandId"),
        QStringLiteral("commandVersion"),QStringLiteral("parameters"),
        QStringLiteral("selectionSnapshot"),QStringLiteral("agentProposal"),
        QStringLiteral("approvalState"),QStringLiteral("preStateHash"),
        QStringLiteral("postStateHash"),QStringLiteral("resultStatus"),
        QStringLiteral("diagnostics"),QStringLiteral("artifactRefs"),
        QStringLiteral("applicationVersion"),QStringLiteral("pluginVersions")};
    if (!exactKeys(object,keys) || !object.value(QStringLiteral("schemaVersion")).isDouble() ||
        object.value(QStringLiteral("schemaVersion")).toDouble() != SchemaVersion)
        return fail(QStringLiteral("Unsupported interaction schema or unknown fields"),error);
    RInteractionRecordV1 candidate;
    if (!readUuid(object,QStringLiteral("recordId"),candidate.recordId,false) ||
        !readUuid(object,QStringLiteral("parentRecordId"),candidate.parentRecordId,true) ||
        !readUuid(object,QStringLiteral("sessionId"),candidate.sessionId,false) ||
        !readUuid(object,QStringLiteral("projectId"),candidate.projectId,false) ||
        !readUuid(object,QStringLiteral("modelId"),candidate.modelId,false) ||
        !readUuid(object,QStringLiteral("runId"),candidate.runId,true))
        return fail(QStringLiteral("Invalid interaction UUID"),error);
    const QJsonValue timestamp = object.value(QStringLiteral("timestamp"));
    if (!timestamp.isString() || !timestampPattern.match(timestamp.toString()).hasMatch())
        return fail(QStringLiteral("Invalid interaction timestamp"),error);
    candidate.timestampUtc = QDateTime::fromString(timestamp.toString(),Qt::ISODateWithMs);
    if (!candidate.timestampUtc.isValid() || candidate.timestampUtc.timeSpec() != Qt::UTC ||
        candidate.timestampUtc.toString(Qt::ISODateWithMs) != timestamp.toString())
        return fail(QStringLiteral("Invalid UTC interaction timestamp"),error);
    for (const QString &key : {QStringLiteral("actorType"),QStringLiteral("actorId"),
         QStringLiteral("modelRevision"),QStringLiteral("workflowStage"),
         QStringLiteral("commandId"),QStringLiteral("commandVersion"),
         QStringLiteral("approvalState"),QStringLiteral("resultStatus"),
         QStringLiteral("applicationVersion")})
        if (!object.value(key).isString())
            return fail(QStringLiteral("Invalid interaction text field: ")+key,error);
    candidate.actorType = object.value(QStringLiteral("actorType")).toString();
    candidate.actorId = object.value(QStringLiteral("actorId")).toString();
    candidate.modelRevision = object.value(QStringLiteral("modelRevision")).toString();
    candidate.workflowStage = object.value(QStringLiteral("workflowStage")).toString();
    candidate.commandId = object.value(QStringLiteral("commandId")).toString();
    candidate.commandVersion = object.value(QStringLiteral("commandVersion")).toString();
    candidate.approvalState = object.value(QStringLiteral("approvalState")).toString();
    candidate.resultStatus = object.value(QStringLiteral("resultStatus")).toString();
    candidate.applicationVersion = object.value(QStringLiteral("applicationVersion")).toString();
    if (!object.value(QStringLiteral("parameters")).isObject() ||
        !object.value(QStringLiteral("selectionSnapshot")).isObject() ||
        !(object.value(QStringLiteral("agentProposal")).isNull() ||
          object.value(QStringLiteral("agentProposal")).isObject()) ||
        !object.value(QStringLiteral("diagnostics")).isArray() ||
        !object.value(QStringLiteral("artifactRefs")).isArray() ||
        !object.value(QStringLiteral("pluginVersions")).isObject())
        return fail(QStringLiteral("Invalid interaction structured field"),error);
    candidate.parameters = object.value(QStringLiteral("parameters")).toObject();
    candidate.selectionSnapshot = object.value(QStringLiteral("selectionSnapshot")).toObject();
    candidate.agentProposal = object.value(QStringLiteral("agentProposal"));
    candidate.diagnostics = object.value(QStringLiteral("diagnostics")).toArray();
    candidate.artifactRefs = object.value(QStringLiteral("artifactRefs")).toArray();
    candidate.pluginVersions = object.value(QStringLiteral("pluginVersions")).toObject();
    for (const auto &pair : {qMakePair(QStringLiteral("preStateHash"),&candidate.preStateHash),
                              qMakePair(QStringLiteral("postStateHash"),&candidate.postStateHash)})
    {
        const QJsonValue value = object.value(pair.first);
        if (!value.isNull() && !value.isString())
            return fail(QStringLiteral("Invalid interaction hash field"),error);
        *pair.second = value.isNull() ? QString() : value.toString();
    }
    if (!candidate.validate(error)) return false;
    record = std::move(candidate);
    return true;
}
