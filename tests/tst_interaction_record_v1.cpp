#include <QtTest>

#include <QJsonDocument>

#include <limits>

#include "rbl_interaction_record_v1.h"

namespace
{
RInteractionRecordV1 validRecord()
{
    RInteractionRecordV1 record;
    record.recordId = QUuid::createUuid();
    record.timestampUtc = QDateTime::fromString(QStringLiteral("2026-09-24T12:34:56.789Z"),
                                                Qt::ISODateWithMs);
    record.actorType = QStringLiteral("agent");
    record.actorId = QStringLiteral("copilot-1");
    record.sessionId = QUuid::createUuid();
    record.projectId = QUuid::createUuid();
    record.modelId = QUuid::createUuid();
    record.modelRevision = QStringLiteral("mesh-7");
    record.workflowStage = QStringLiteral("control");
    record.commandId = QStringLiteral("control.set-gain");
    record.commandVersion = QStringLiteral("1.0.0");
    record.parameters = {{QStringLiteral("gain"),2.0}};
    record.selectionSnapshot = {{QStringLiteral("actuatorIds"),QJsonArray{QStringLiteral("tip-force")}}};
    record.agentProposal = QJsonObject{{QStringLiteral("reason"),QStringLiteral("damping")}};
    record.approvalState = QStringLiteral("approved");
    record.resultStatus = QStringLiteral("succeeded");
    record.diagnostics = {QJsonObject{{QStringLiteral("code"),QStringLiteral("applied")},
                                      {QStringLiteral("severity"),QStringLiteral("info")},
                                      {QStringLiteral("message"),QStringLiteral("validated")}}};
    record.artifactRefs = {QJsonObject{{QStringLiteral("artifactId"),QStringLiteral("result-1")},
                                       {QStringLiteral("sha256"),QString(64,QLatin1Char('a'))}}};
    record.applicationVersion = QStringLiteral("1.3.10");
    record.pluginVersions = {{QStringLiteral("controller.pid"),QStringLiteral("1.0.0")}};
    return record;
}

QJsonObject objectFor(const RInteractionRecordV1 &record)
{
    QByteArray bytes;
    QString error;
    if (!record.toJson(bytes,&error)) qFatal("Invalid fixture: %s",qPrintable(error));
    return QJsonDocument::fromJson(bytes).object();
}

bool parseObject(const QJsonObject &object, RInteractionRecordV1 &record, QString &error)
{
    return RInteractionRecordV1::fromJson(QJsonDocument(object).toJson(QJsonDocument::Compact),
                                          record,&error);
}
}

class TestInteractionRecordV1 : public QObject
{
    Q_OBJECT
private slots:
    void roundTripsRequiredFieldsAndExplicitNulls();
    void rejectsBadEnvelope_data();
    void rejectsBadEnvelope();
    void rejectsNonFiniteNestedValueAndBoundaries();
    void parseFailureDoesNotMutateTarget();
};

void TestInteractionRecordV1::roundTripsRequiredFieldsAndExplicitNulls()
{
    RInteractionRecordV1 source = validRecord();
    QByteArray bytes;
    QString error;
    QVERIFY2(source.toJson(bytes,&error),qPrintable(error));
    const QJsonObject json = QJsonDocument::fromJson(bytes).object();
    QCOMPARE(json.value(QStringLiteral("schemaVersion")).toInt(),1);
    QVERIFY(json.value(QStringLiteral("parentRecordId")).isNull());
    QVERIFY(json.value(QStringLiteral("runId")).isNull());
    QVERIFY(json.value(QStringLiteral("preStateHash")).isNull());
    QVERIFY(json.value(QStringLiteral("postStateHash")).isNull());
    RInteractionRecordV1 parsed;
    QVERIFY2(RInteractionRecordV1::fromJson(bytes,parsed,&error),qPrintable(error));
    QCOMPARE(parsed.recordId,source.recordId);
    QCOMPARE(parsed.timestampUtc,source.timestampUtc);
    QCOMPARE(parsed.agentProposal.toObject(),source.agentProposal.toObject());
    QCOMPARE(parsed.artifactRefs,source.artifactRefs);
    parsed.parentRecordId = QUuid::createUuid();
    parsed.runId = QUuid::createUuid();
    parsed.preStateHash = QString(64,QLatin1Char('b'));
    parsed.postStateHash = QString(64,QLatin1Char('c'));
    parsed.agentProposal = QJsonValue(QJsonValue::Null);
    QVERIFY2(parsed.toJson(bytes,&error),qPrintable(error));
    RInteractionRecordV1 reread;
    QVERIFY2(RInteractionRecordV1::fromJson(bytes,reread,&error),qPrintable(error));
    QCOMPARE(reread.parentRecordId,parsed.parentRecordId);
    QCOMPARE(reread.runId,parsed.runId);
    QCOMPARE(reread.postStateHash,parsed.postStateHash);
    QVERIFY(reread.agentProposal.isNull());
}

void TestInteractionRecordV1::rejectsBadEnvelope_data()
{
    QTest::addColumn<QString>("field");
    QTest::addColumn<QJsonValue>("badValue");
    QTest::newRow("future schema") << QStringLiteral("schemaVersion") << QJsonValue(2);
    QTest::newRow("fractional schema") << QStringLiteral("schemaVersion") << QJsonValue(1.5);
    QTest::newRow("uuid") << QStringLiteral("recordId") << QJsonValue(QStringLiteral("not-uuid"));
    QTest::newRow("zero uuid") << QStringLiteral("recordId") << QJsonValue(QStringLiteral("00000000-0000-0000-0000-000000000000"));
    QTest::newRow("parent type") << QStringLiteral("parentRecordId") << QJsonValue(12);
    QTest::newRow("timezone") << QStringLiteral("timestamp") << QJsonValue(QStringLiteral("2026-09-24T12:34:56.789+08:00"));
    QTest::newRow("invalid date") << QStringLiteral("timestamp") << QJsonValue(QStringLiteral("2026-02-30T12:34:56.789Z"));
    QTest::newRow("actor") << QStringLiteral("actorType") << QJsonValue(QStringLiteral("untrusted"));
    QTest::newRow("command version") << QStringLiteral("commandVersion") << QJsonValue(QStringLiteral("latest"));
    QTest::newRow("approval") << QStringLiteral("approvalState") << QJsonValue(QStringLiteral("auto-approved"));
    QTest::newRow("hash") << QStringLiteral("preStateHash") << QJsonValue(QStringLiteral("sha256:abc"));
    QTest::newRow("parameters type") << QStringLiteral("parameters") << QJsonValue(QJsonArray{});
    QTest::newRow("proposal type") << QStringLiteral("agentProposal") << QJsonValue(QStringLiteral("text"));
    QTest::newRow("plugin version") << QStringLiteral("pluginVersions") << QJsonValue(QJsonObject{{QStringLiteral("plugin"),12}});
    QTest::newRow("diagnostic") << QStringLiteral("diagnostics") << QJsonValue(QJsonArray{QJsonObject{{QStringLiteral("message"),QStringLiteral("missing code")}}});
    QTest::newRow("artifact") << QStringLiteral("artifactRefs") << QJsonValue(QJsonArray{QJsonObject{{QStringLiteral("artifactId"),QStringLiteral("a")},{QStringLiteral("sha256"),QStringLiteral("bad")}}});
}

void TestInteractionRecordV1::rejectsBadEnvelope()
{
    QFETCH(QString,field);
    QFETCH(QJsonValue,badValue);
    QJsonObject json = objectFor(validRecord());
    json.insert(field,badValue);
    RInteractionRecordV1 parsed;
    QString error;
    QVERIFY(!parseObject(json,parsed,error));
    QVERIFY(!error.isEmpty());
}

void TestInteractionRecordV1::rejectsNonFiniteNestedValueAndBoundaries()
{
    RInteractionRecordV1 source = validRecord();
    QString error;
    source.parameters.insert(QStringLiteral("bad"),std::numeric_limits<double>::infinity());
    QByteArray output("unchanged");
    QVERIFY(!source.toJson(output,&error));
    QCOMPARE(output,QByteArray("unchanged"));
    source = validRecord();
    source.parameters.insert(QStringLiteral("bad"),QJsonValue(QJsonValue::Null));
    QVERIFY(!source.toJson(output,&error));
    source = validRecord();
    source.parameters.insert(QStringLiteral("large"),QString(300000,QLatin1Char('x')));
    QVERIFY(!source.toJson(output,&error));
    source = validRecord();
    for (int i=0;i<100;i++)
        source.parameters.insert(QStringLiteral("payload-%1").arg(i),
                                 QString(3000,QLatin1Char('x')));
    QVERIFY(!source.toJson(output,&error));
    QCOMPARE(output,QByteArray("unchanged"));
    QJsonObject json = objectFor(validRecord());
    json.insert(QStringLiteral("unknownField"),true);
    RInteractionRecordV1 parsed;
    QVERIFY(!parseObject(json,parsed,error));
    json = objectFor(validRecord());
    json.remove(QStringLiteral("actorId"));
    QVERIFY(!parseObject(json,parsed,error));
    QVERIFY(!RInteractionRecordV1::fromJson(QByteArray(300000,'x'),parsed,&error));
}

void TestInteractionRecordV1::parseFailureDoesNotMutateTarget()
{
    RInteractionRecordV1 target = validRecord();
    const QUuid original = target.recordId;
    QJsonObject json = objectFor(validRecord());
    json.insert(QStringLiteral("applicationVersion"),QStringLiteral("invalid"));
    QString error;
    QVERIFY(!parseObject(json,target,error));
    QCOMPARE(target.recordId,original);
}

QTEST_APPLESS_MAIN(TestInteractionRecordV1)
#include "tst_interaction_record_v1.moc"
