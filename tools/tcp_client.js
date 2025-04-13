const net = require('net');

// 定义服务器地址和端口
const HOST = '192.168.31.5'; // 替换为实际服务器地址
const PORT = 12345;           // 替换为实际端口

// 创建 Socket 客户端
const client = new net.Socket();

// 连接到服务器
client.connect(PORT, HOST, () => {
  console.log(`Connected to ${HOST}:${PORT}`);

  // 发送数据（示例：发送 HTTP GET 请求）
  // client.write('GET / HTTP/1.1\r\nHost: example.com\r\n\r\n');
});

// 接收服务器响应
client.on('data', (data) => {
  console.log('Received:\n', data.toString());
  // client.end(); // 关闭连接
});

// 处理连接关闭
client.on('close', () => {
  console.log('Connection closed');
});

// 处理错误
client.on('error', (err) => {
  console.error('Error:', err.message);
});
