# message pass

[toc]

---

## 目标

传递daq数据，并保证高可用

## 消息传递流程

### 获取data

1. sink发送request消息到kafka
2. source从kafka拿到request（request消息中含有sink地址）
3. source按照request消息中的sink地址，用zmq发送物理数据

```seq
sink->kafka: GET_DATA_REQUEST
kafka->source: GET_DATA_REQUEST
source->sink: DATA
```

### 删除data

```seq
sink->kafka: DEL_DATA_REQUEST
kafka->source: DEL_DATA_REQUEST
source->source: DEL_OPERATION
```

### 优势

1. 利用kafka作为request消息传递中间件，保证request消息可靠性
2. 物理数据使用zmq发送和接收，保证速度
3. 简化消息接收和发送流程
4. 高可用：sink端故障后，通过recover request可以再次获取未删除数据；即使kafka丢失消息，也可以通过简单的重新发送request请求，让系统继续工作（kafka故障时会保证消费者消费一致性）；综上，能够保证daq数据流仅在读出节点故障时才需要重启全部进程

## 依赖

1. Boost thread
2. jsoncpp
3. Protobuf
4. spdlog
5. librdkafka
6. zmq

## 测试用例

tests/*

## TODO

- [x] recover and delete
- [x] fixed message size send and recv
- [ ] metrics
- [ ] 用户设置日志