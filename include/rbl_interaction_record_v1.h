#ifndef RBL_INTERACTION_RECORD_V1_H
#define RBL_INTERACTION_RECORD_V1_H

#include <QByteArray>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QUuid>

//! Versioned interaction data contract. Storage and GUI/Agent hooks are separate.
struct RInteractionRecordV1
{
    static constexpr int SchemaVersion = 1;
    static constexpr int MaxJsonBytes = 256*1024;

    QUuid recordId;
    QUuid parentRecordId;              //!< Null only for a root record.
    QDateTime timestampUtc;
    QString actorType;
    QString actorId;
    QUuid sessionId;
    QUuid projectId;
    QUuid modelId;
    QString modelRevision;
    QUuid runId;                       //!< Null before a run exists.
    QString workflowStage;
    QString commandId;
    QString commandVersion;
    QJsonObject parameters;
    QJsonObject selectionSnapshot;
    QJsonValue agentProposal = QJsonValue(QJsonValue::Null); //!< Object or null.
    QString approvalState;
    QString preStateHash;              //!< Empty means JSON null/unavailable.
    QString postStateHash;             //!< Empty means JSON null/unavailable.
    QString resultStatus;
    QJsonArray diagnostics;            //!< {code,severity,message} objects.
    QJsonArray artifactRefs;           //!< {artifactId,sha256} objects.
    QString applicationVersion;
    QJsonObject pluginVersions;        //!< Plugin ID to semantic version.

    bool validate(QString *error = nullptr) const;
    bool toJson(QByteArray &bytes, QString *error = nullptr) const;
    static bool fromJson(const QByteArray &bytes, RInteractionRecordV1 &record,
                         QString *error = nullptr);
};

#endif // RBL_INTERACTION_RECORD_V1_H
