let bleDevice;
let gattServer;
let uartService, uartRXCharacteristic, uartTXCharacteristic;
let timeService, timeCharacteristic;

let canvas, ctx;
let startTime;

const BLE_UUID_CURRENT_TIME_SERVICE = 0x1805;
const BLE_UUID_CURRENT_TIME_CHAR = 0x2A2B;
const BLE_UUID_UART_SERVICE = "6e400001-b5a3-f393-e0a9-e50e24dcca9e";
const BLE_UUID_UART_RX_CHAR = "6e400002-b5a3-f393-e0a9-e50e24dcca9e";
const BLE_UUID_UART_TX_CHAR = "6e400003-b5a3-f393-e0a9-e50e24dcca9e";

const MAX_PACKET_SIZE = 20;

function resetVariables() {
  gattServer = null;
  uartService = uartRXCharacteristic = uartTXCharacteristic = null;
  timeService = timeCharacteristic = null;
  document.getElementById("log").value = '';
}

async function write(data, status = false) {
  if (uartRXCharacteristic == null) {
    addLog("UART 服务不可用，请检查蓝牙连接");
    return false;
  }

  let isHex = false;
  let bytes = data;
  if (typeof data === 'string') {
    bytes = new TextEncoder().encode(data);
  } else if (data instanceof Uint8Array) {
    isHex = true;
  } else {
    throw new Error("Unsupported data type: " + typeof data);
  }

  const chunkSize = MAX_PACKET_SIZE;
  const count = Math.round(data.length / chunkSize);
  const interleavedCount = document.getElementById('interleavedcount').value;
  let chunkIdx = 1;
  let noReplyCount = interleavedCount;

  for (let i = 0; i < bytes.length; i += chunkSize) {
    const part = bytes.slice(i, i + chunkSize);
    if (status && startTime) {
      let currentTime = (Date.now() - startTime) / 1000.0;
      setStatus(`数据块: ${chunkIdx}/${count}, 总用时: ${currentTime}s`);
    }
    addLog(`<span class="action">⇑</span> ${isHex ? bytes2hex(part) : data}`);
    if (noReplyCount > 0) {
      await uartRXCharacteristic.writeValueWithoutResponse(part);
      noReplyCount--;
    } else {
      await uartRXCharacteristic.writeValueWithResponse(part);
      noReplyCount = interleavedCount;
    }
    chunkIdx++;
  }

  return true;
}

function encodeBitmap(depth) {
  const width = canvas.width;
  const height = canvas.height;
  const imageData = ctx.getImageData(0, 0, width, height);

  const hdr = new DataView(new ArrayBuffer(7));
  hdr.setUint8(0, 0x21); // !
  hdr.setUint8(1, 0x49); // I
  hdr.setUint8(2, depth); // 16
  hdr.setUint16(3, canvas.width, true);
  hdr.setUint16(5, canvas.height, true);

  if (depth == 16) {
    const bitmap = new Uint16Array(width * height);
    for (let i = 0; i < imageData.data.length; i += 4) {
      const r = imageData.data[i];
      const g = imageData.data[i + 1];
      const b = imageData.data[i + 2];
      bitmap[i / 4] = (r & 0xf8) << 8 | (g & 0xfc) << 3 | b >> 3;
    }
    return new Uint8Array([...new Uint8Array(hdr.buffer), ...new Uint8Array(bitmap.buffer)]);
  } else if (depth == 1) {
    const bitmap = new Uint8Array(Math.ceil(width * height / 8));
    for (let i = 0; i < imageData.data.length; i += 4) {
      const r = imageData.data[i];
      const g = imageData.data[i + 1];
      const b = imageData.data[i + 2];
      const idx = i / 4;
      const bit = (r === 0 && g === 0 && b === 0) ? 0 : 1;
      const byteIdx = Math.floor(idx / 8);
      const bitIdx = 7 - (idx % 8);
      bitmap[byteIdx] |= bit << bitIdx
    }
    return new Uint8Array([...new Uint8Array(hdr.buffer), ...bitmap]);
  } else if (depth == 2) {
    const bitmap = new Uint8Array(Math.ceil(width * height / 4));
    for (let i = 0; i < imageData.data.length; i += 4) {
      const r = imageData.data[i];
      const g = imageData.data[i + 1];
      const b = imageData.data[i + 2];
      const idx = i / 4;
      let bit = (r === 0 && g === 0 && b === 0) ? 0 : 1;
      if (r > 0 && g === 0 && b === 0) bit = 2;
      const byteIdx = Math.floor(idx / 4);
      const bitIdx = 6 - (idx % 4) * 2;
      bitmap[byteIdx] |= bit << bitIdx;
    }
    return new Uint8Array([...new Uint8Array(hdr.buffer), ...bitmap]);
  } else {
    throw new Error("Unsupported depth: " + depth);
  }
}

async function sendimg() {
  const mode = document.getElementById('dithering').value;
  const depth = mode.startsWith("bwr") ? 2 : 1;
  startTime = Date.now();
  if (await write(encodeBitmap(depth), true)) {
    const sendTime = (Date.now() - startTime) / 1000.0;
    addLog(`发送完成！耗时: ${sendTime}s`);
    setStatus(`发送完成！耗时: ${sendTime}s`);
  }
}

function updateButtonStatus() {
  const connected = (gattServer != null && gattServer.connected);
  const uartStatus = (connected && uartRXCharacteristic != null) ? null : 'disabled';
  const timeStatus = (connected && timeCharacteristic != null) ? null : 'disabled';
  document.getElementById("reconnectbutton").disabled = (gattServer == null || gattServer.connected) ? 'disabled' : null;
  document.getElementById("sendimgbutton").disabled = uartStatus;
  document.getElementById("synctimebutton").disabled = timeStatus;
}

function disconnect() {
  updateButtonStatus();
  resetVariables();
  addLog('已断开连接.');
  document.getElementById("connectbutton").innerHTML = '连接';
}

async function preConnect() {
  if (gattServer != null && gattServer.connected) {
    if (bleDevice != null && bleDevice.gatt.connected) {
      bleDevice.gatt.disconnect();
    }
  } else {
    connectTrys = 0;
    try {
      bleDevice = await navigator.bluetooth.requestDevice({
        optionalServices: [BLE_UUID_CURRENT_TIME_SERVICE, BLE_UUID_UART_SERVICE],
        acceptAllDevices: true
      });
    } catch (e) {
      if (e.message) addLog(e.message);
      return;
    }

    await bleDevice.addEventListener('gattserverdisconnected', disconnect);
    await connect();
  }
}

async function reConnect() {
  connectTrys = 0;
  if (bleDevice != null && bleDevice.gatt.connected)
    bleDevice.gatt.disconnect();
  resetVariables();
  addLog("正在重连");
  setTimeout(async function () { await connect(); }, 300);
}

async function clearscreen() {
  if (confirm('确认清除屏幕内容?')) {
    write("clear");
  }
}

async function syncTime() {
  if (timeCharacteristic == null) {
    addLog("时间服务不可用，请检查蓝牙连接");
    return;
  }

  addLog("正在同步时间...");
  const now = new Date();
  const timeValue = new DataView(new ArrayBuffer(7));
  timeValue.setUint16(0, now.getFullYear(), true);
  timeValue.setUint8(2, now.getMonth() + 1);
  timeValue.setUint8(3, now.getDate());
  timeValue.setUint8(4, now.getHours());
  timeValue.setUint8(5, now.getMinutes());
  timeValue.setUint8(6, now.getSeconds());
  await timeCharacteristic.writeValue(timeValue.buffer);
  addLog("时间同步完成！");

  const value = await timeCharacteristic.readValue();
  const year = value.getUint16(0, true);
  const month = value.getUint8(2);
  const day = value.getUint8(3);
  const hours = value.getUint8(4);
  const minutes = value.getUint8(5);
  const seconds = value.getUint8(6);
  addLog(`远端时间: ${year}/${month}/${day} ${hours}:${minutes}:${seconds}`);
  addLog(`本地时间: ${new Date().toLocaleString()}`);

  addLog("日历模式已开启。");
}

async function connect() {
  if (bleDevice != null) {
    addLog("正在连接: " + bleDevice.name);

    gattServer = await bleDevice.gatt.connect();
    addLog('  找到 GATT Server');

    try {
      timeService = await gattServer.getPrimaryService(BLE_UUID_CURRENT_TIME_SERVICE);
      addLog('  找到 Time Service');
      timeCharacteristic = await timeService.getCharacteristic(BLE_UUID_CURRENT_TIME_CHAR);
      addLog('  找到 Time Characteristic');
    } catch (e) {
      console.error(e);
      addLog("  未找到 Time Service");
      timeService = timeCharacteristic = null;
    }

    try {
      uartService = await gattServer.getPrimaryService(BLE_UUID_UART_SERVICE);
      addLog('  找到 UART Service');
      uartRXCharacteristic = await uartService.getCharacteristic(BLE_UUID_UART_RX_CHAR);
      addLog('  找到 UART RX Characteristic');
      uartTXCharacteristic = await uartService.getCharacteristic(BLE_UUID_UART_TX_CHAR);
      addLog('  找到 UART TX Characteristic');

      await uartTXCharacteristic.startNotifications();
      uartTXCharacteristic.addEventListener('characteristicvaluechanged', (event) => {
        const msg = new TextDecoder().decode(event.target.value);
        addLog(`<span class="action">⇓</span> ${msg}`);
      });
    } catch (e) {
      console.error(e);
      addLog("  未找到 UART Service");
      uartService = uartRXCharacteristic = uartTXCharacteristic = null;
    }

    document.getElementById("connectbutton").innerHTML = '断开';
    updateButtonStatus();
  }
}

function setStatus(statusText) {
  document.getElementById("status").innerHTML = statusText;
}

function addLog(logTXT) {
  const log = document.getElementById("log");
  const now = new Date();
  const time = String(now.getHours()).padStart(2, '0') + ":" +
    String(now.getMinutes()).padStart(2, '0') + ":" +
    String(now.getSeconds()).padStart(2, '0') + " ";
  log.innerHTML += '<span class="time">' + time + '</span>' + logTXT + '<br>';
  log.scrollTop = log.scrollHeight;
  while ((log.innerHTML.match(/<br>/g) || []).length > 20) {
    var logs_br_position = log.innerHTML.search("<br>");
    log.innerHTML = log.innerHTML.substring(logs_br_position + 4);
    log.scrollTop = log.scrollHeight;
  }
}

function clearLog() {
  document.getElementById("log").innerHTML = '';
}

function hex2bytes(hex) {
  for (var bytes = [], c = 0; c < hex.length; c += 2)
    bytes.push(parseInt(hex.substr(c, 2), 16));
  return new Uint8Array(bytes);
}

function bytes2hex(data) {
  return new Uint8Array(data).reduce(
    function (memo, i) {
      return memo + ("0" + i.toString(16)).slice(-2);
    }, "");
}

async function update_image() {
  let image = new Image();;
  const image_file = document.getElementById('image_file');
  if (image_file.files.length > 0) {
    const file = image_file.files[0];
    image.src = URL.createObjectURL(file);
  } else {
    image.src = document.getElementById('demo-img').src;
  }

  image.onload = function (event) {
    URL.revokeObjectURL(this.src);
    ctx.drawImage(image, 0, 0, image.width, image.height, 0, 0, canvas.width, canvas.height);
    convert_dithering()
  }
}

function clear_canvas() {
  if (confirm('确认清除画布内容?')) {
    ctx.fillStyle = 'white';
    ctx.fillRect(0, 0, canvas.width, canvas.height);
  }
}

function convert_dithering() {
  const mode = document.getElementById('dithering').value;
  if (mode.startsWith('bwr')) {
    ditheringCanvasByPalette(canvas, bwrPalette, mode);
  } else {
    dithering(ctx, canvas.width, canvas.height, parseInt(document.getElementById('threshold').value), mode);
  }
}

document.body.onload = () => {
  canvas = document.getElementById('canvas');
  ctx = canvas.getContext("2d");

  updateButtonStatus();
  update_image();

  document.getElementById('dithering').value = 'none';
}