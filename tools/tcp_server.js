const net = require('net');
const PORT = 1234;

// 创建 TCP 服务器
const server = net.createServer((socket) => {
  // 新客户端连接时触发
  console.log(`[${new Date().toISOString()}] Client connected from ${socket.remoteAddress}:${socket.remotePort}`);

  // 接收客户端数据
  socket.on('data', (data) => {
    const message = data.toString().trim();
    console.log(`[${socket.remoteAddress}] Received: ${message}`);

    // 回显数据给客户端（示例）
    socket.write(`Server reply: ${message.toUpperCase()}\n`);

    // 特定指令处理（例如关闭连接）
    if (message === 'EXIT') {
      socket.end('Goodbye!\n');
    }
  });

  // 客户端主动断开连接
  socket.on('end', () => {
    console.log(`[${socket.remoteAddress}] Connection closed`);
  });

  // 错误处理
  socket.on('error', (err) => {
    console.error(`[${socket.remoteAddress}] Error: ${err.message}`);
  });
});

// 启动服务器监听
server.listen(PORT, () => {
  console.log(`Server running at http://localhost:${PORT}/`);
});

// 处理服务器级错误
server.on('error', (err) => {
  console.error(`Server error: ${err.message}`);
});
