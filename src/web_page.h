#pragma once
// Dashboard page served on port 80. It opens a WebSocket on port 81
// (8181 when running in Wokwi, see wokwi.toml), shows the latest $GS
// telemetry line and keeps a fault history list.

static const char WEB_PAGE[] = R"HTML(
<html>
  <body>
    <h1>Groundstation</h1>

    <button onclick="sendCmd('W')">Forward (W)</button><br><br>
    <button onclick="sendCmd('A')">Left (A)</button>
    <button onclick="sendCmd('STOP')">STOP</button>
    <button onclick="sendCmd('D')">Right (D)</button><br><br>
    <button onclick="sendCmd('S')">Back (S)</button>

    <h2>State: <span id="state">?</span> &nbsp; Fault: <span id="fault">-</span></h2>
    <button onclick="sendCmd('ACK')">ACK Fault</button>
    <button onclick="sendCmd('RESET')">RESET System</button>

    <pre id="tele">waiting for telemetry...</pre>
    <h3>Fault history</h3>
    <ol id="history"></ol>

    <script>
      // Wokwi forwards 8180->80 and 8181->81, a real board uses 81
      var wsPort = (location.port === '8180') ? 8181 : 81;
      var connection = new WebSocket('ws://' + location.hostname + ':' + wsPort + '/');
      var names = ['seq','t_ms','pitch','roll','spd_l','spd_r','pwm_l','pwm_r','dist_cm',
                   'temp_c','state','fault','warn','faults','loop_avg_us','loop_max_us','period_max_us'];
      var lastFault = '-';

      function sendCmd(cmd) {
        if (connection.readyState === WebSocket.OPEN) connection.send(cmd);
      }

      // WASD keys, space = stop
      document.addEventListener('keydown', function(e) {
        var k = e.key.toUpperCase();
        if ('WASD'.indexOf(k) >= 0 && k.length === 1) sendCmd(k);
        if (e.key === ' ') sendCmd('STOP');
      });

      // ping keeps the F04 watchdog happy (limit is 500 ms)
      setInterval(function() { sendCmd('ping'); }, 200);

      // $GS,seq,...*CS -> show each field, add new faults to the history
      connection.onmessage = function(event) {
        if (!event.data.startsWith('$GS,')) return;
        var fields = event.data.split('*')[0].split(',').slice(1);
        var text = '';
        for (var i = 0; i < names.length; i++) text += names[i] + ': ' + fields[i] + '\n';
        document.getElementById('tele').innerText = text;
        document.getElementById('state').innerText = fields[10];
        document.getElementById('fault').innerText = fields[11];
        if (fields[11] !== '-' && fields[11] !== lastFault) {
          var li = document.createElement('li');
          li.innerText = fields[11] + ' at ' + fields[1] + ' ms';
          document.getElementById('history').appendChild(li);
        }
        lastFault = fields[11];
      };
    </script>
  </body>
</html>
)HTML";
