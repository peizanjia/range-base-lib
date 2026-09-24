# InteractionRecord v1 数据契约

`RInteractionRecordV1` 是追加式交互记录的数据结构和 JSON 解析/序列化契约，不是持久化 journal，也没有自动挂接 GUI、Agent 或求解器。调用方需自行决定何时写入、是否需要事务、索引、回放及保留策略。

所有字段都必须出现在 JSON 中；`schemaVersion` 必须是整数 `1`。下列字段允许显式 `null`，其余字段不允许缺失或 `null`：

| 字段 | `null` 语义 | C++ 表示 |
| --- | --- | --- |
| `parentRecordId` | 根记录 | 空 `QUuid` |
| `runId` | 尚未建立求解 run | 空 `QUuid` |
| `agentProposal` | 没有 Agent 提案 | `QJsonValue::Null` |
| `preStateHash`、`postStateHash` | 状态摘要不可用 | 空 `QString` |

`recordId`、`sessionId`、`projectId`、`modelId` 为非零 UUID；`timestamp` 为精确到毫秒、带 `Z` 的 UTC ISO 8601 字符串。`commandVersion`、`applicationVersion` 和 `pluginVersions` 的版本值使用 `major.minor.patch`；哈希是 64 位小写十六进制 SHA-256 文本。`diagnostics` 的每项必须有 `code`、`severity`（`info/warning/error`）和 `message`；`artifactRefs` 每项必须有 `artifactId`、`sha256`，不嵌入工件内容。

`parameters`、`selectionSnapshot`、非空 `agentProposal` 可以容纳命令特定 JSON，但深度不超过 16、数组/对象单层不超过 1024 项、单个字符串不超过 4096 字符，不接受 `null` 或非有限数值。整条 JSON 限 256 KiB。未知顶层字段、未来主模式版本、错误类型和无效时间/UUID/版本均被拒绝。解析失败不会覆盖调用方已有的记录对象；序列化失败不会覆盖输出字节。

调用方必须在进入本契约前过滤 API 密钥、令牌、口令和不受控文本；本类不能从任意字符串可靠识别秘密。跨记录顺序、父子引用存在性、工件真实哈希、命令授权、Undo/Redo、磁盘原子性和可回放性也不由本类保证。
