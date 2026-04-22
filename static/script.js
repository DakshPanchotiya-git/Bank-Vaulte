// let modalOpen = false;
// let lastDist = 0;
// const logWindow = document.getElementById('logWindow');

// function addLog(message, type = 'system') {
//   const time = new Date().toLocaleTimeString([], { hour12: false });
//   const p = document.createElement('p');
//   p.className = `log-entry ${type}`;
//   p.innerText = `[${time}] ${message}`;
//   logWindow.appendChild(p);
//   logWindow.scrollTop = logWindow.scrollHeight; 
// }

// setInterval(async () => {
//   try {
//     let res = await fetch('/api/get_state');
//     let data = await res.json();
    
//     if (Math.abs(data.distance - lastDist) > 5) {
//       document.getElementById('distance').innerText = data.distance + " cm";
//       addLog(`Motion detected at ${data.distance}cm`);
//       lastDist = data.distance;
//     }

//     if (data.ir_triggered && !modalOpen) {
//       document.getElementById('authModal').style.display = 'block';
//       document.getElementById('clientId').value = '';
//       document.getElementById('clientPass').value = '';
//       document.getElementById('statusMsg').innerText = '';
//       addLog("BIOMETRIC SENSOR TRIGGERED. Awaiting credentials...", "alert");
//       document.getElementById('sysStatus').innerText = "AUTHORIZATION REQUIRED";
//       document.getElementById('sysStatus').style.color = "var(--alert)";
//       modalOpen = true;
//     }
//   } catch (e) {}
// }, 500);

// async function attemptLogin() {
//   let id = document.getElementById('clientId').value;
//   let pass = document.getElementById('clientPass').value;
//   let msgBox = document.getElementById('statusMsg');

//   addLog(`Auth attempt for ID: ${id}...`, "system");

//   let res = await fetch('/api/login', {
//     method: 'POST',
//     headers: { 'Content-Type': 'application/json' },
//     body: JSON.stringify({ id: id, password: pass })
//   });
//   let data = await res.json();

//   if (data.success) {
//     msgBox.style.color = "var(--accent)";
//     addLog(`ACCESS GRANTED. Vault ${data.vault_no} opening.`, "success");
//     document.getElementById('modalBox').innerHTML = `
//       <h2 style="color:var(--accent);">ACCESS GRANTED</h2>
//       <p style="color:#fff;">Vault No. <b style="color:var(--accent);">${data.vault_no}</b> is opened.</p>
//     `;
//     setTimeout(() => {
//       document.getElementById('authModal').style.display = 'none';
//       modalOpen = false;
//       location.reload(); 
//     }, 4000);
//   } else {
//     msgBox.style.color = "var(--alert)";
//     msgBox.innerText = data.error;
//     addLog(`Auth failed: ${data.error}`, "alert");
//   }
// }
// ===== STATE =====
let modalOpen = false;
let lastDist = -1;
let entryCount = 0;
let exitCount = 0;
let peopleInside = 0;
let failedToday = 0;
let logEventCount = 0;
let startTime = Date.now();
let vaultIsOpen = false;

// ===== CLOCK & UPTIME =====
function updateClock() {
  const now = new Date();
  const hh = String(now.getHours()).padStart(2,'0');
  const mm = String(now.getMinutes()).padStart(2,'0');
  const ss = String(now.getSeconds()).padStart(2,'0');
  document.getElementById('clockDisplay').textContent = `${hh}:${mm}:${ss}`;
  const days = ['SUNDAY','MONDAY','TUESDAY','WEDNESDAY','THURSDAY','FRIDAY','SATURDAY'];
  const months = ['JAN','FEB','MAR','APR','MAY','JUN','JUL','AUG','SEP','OCT','NOV','DEC'];
  document.getElementById('dateDisplay').textContent =
    `${days[now.getDay()]} · ${String(now.getDate()).padStart(2,'0')} ${months[now.getMonth()]} ${now.getFullYear()}`;

  const elapsed = Math.floor((Date.now() - startTime) / 1000);
  const uh = String(Math.floor(elapsed / 3600)).padStart(2,'0');
  const um = String(Math.floor((elapsed % 3600) / 60)).padStart(2,'0');
  const us = String(elapsed % 60).padStart(2,'0');
  document.getElementById('uptimeDisplay').textContent = `${uh}:${um}:${us}`;
}
setInterval(updateClock, 1000);
updateClock();

// ===== LOG =====
function addLog(message, type = 'system') {
  const logWindow = document.getElementById('logWindow');
  const now = new Date();
  const time = `${String(now.getHours()).padStart(2,'0')}:${String(now.getMinutes()).padStart(2,'0')}:${String(now.getSeconds()).padStart(2,'0')}`;

  const tagMap = { system: 'SYS', alert: 'ALRT', success: 'AUTH', motion: 'MOT' };
  const tagClass = { system: 'sys-tag', alert: 'alert-tag', success: 'success-tag', motion: 'motion-tag' };

  const entry = document.createElement('div');
  entry.className = `log-entry ${type}`;
  entry.innerHTML = `
    <span class="log-time">${time}</span>
    <span class="log-tag ${tagClass[type] || 'sys-tag'}">${tagMap[type] || 'SYS'}</span>
    <span class="log-msg">${message}</span>
  `;
  logWindow.appendChild(entry);
  logWindow.scrollTop = logWindow.scrollHeight;

  logEventCount++;
  document.getElementById('logCount').textContent = `${logEventCount} events`;

  // Keep log trimmed to last 100 entries
  while (logWindow.children.length > 100) logWindow.removeChild(logWindow.firstChild);
}

function clearLog() {
  document.getElementById('logWindow').innerHTML = '';
  logEventCount = 0;
  document.getElementById('logCount').textContent = '0 events';
  addLog('Log cleared by operator.', 'system');
}

// ===== TIMELINE =====
function addTimelineEvent(userId, vaultNo, success) {
  const timeline = document.getElementById('timeline');
  const empty = timeline.querySelector('.timeline-empty');
  if (empty) empty.remove();

  const now = new Date();
  const time = `${String(now.getHours()).padStart(2,'0')}:${String(now.getMinutes()).padStart(2,'0')}`;

  const item = document.createElement('div');
  item.className = `timeline-item ${success ? 'tl-success' : 'tl-fail'}`;
  item.innerHTML = `
    <div class="timeline-dot"></div>
    <span class="tl-time">${time}</span>
    <span class="tl-user">${userId.toUpperCase()}</span>
    <span class="tl-vault">${success ? vaultNo : 'DENIED'}</span>
  `;
  timeline.insertBefore(item, timeline.firstChild);

  // Keep last 10
  while (timeline.children.length > 10) timeline.removeChild(timeline.lastChild);
}

// ===== PROXIMITY ARC =====
function updateArc(distance) {
  const maxDist = 300;
  const ratio = Math.min(distance / maxDist, 1);
  const totalLen = 251;
  const fill = totalLen * ratio;
  document.getElementById('arcProgress').setAttribute('stroke-dashoffset', totalLen - fill);

  // Needle: maps 0–300cm to -90deg to +90deg
  const angle = -90 + ratio * 180;
  document.getElementById('arcNeedle').style.transform = `rotate(${angle}deg)`;
}

// ===== THREAT LEVEL =====
function updateThreat(distance, irTriggered) {
  let level, pct, color, text;
  if (irTriggered) {
    level = 'CRITICAL'; pct = 95; color = '#ff3333'; text = 'CRITICAL';
  } else if (distance < 50) {
    level = 'HIGH'; pct = 70; color = '#ff7733'; text = 'HIGH';
  } else if (distance < 150) {
    level = 'MODERATE'; pct = 40; color = '#c9a84c'; text = 'MEDIUM';
  } else {
    level = 'LOW'; pct = 15; color = '#00cc6a'; text = 'LOW';
  }
  document.getElementById('threatFill').style.width = pct + '%';
  document.getElementById('threatFill').style.background = color;
  document.getElementById('threatText').textContent = text;
  document.getElementById('threatText').style.color = color;
}

// ===== ATTEMPT INDICATOR =====
function setAttemptPips(attemptsUsed) {
  const pips = document.querySelectorAll('.attempt-pip');
  pips.forEach((pip, i) => {
    pip.className = 'attempt-pip';
    const remaining = 3 - attemptsUsed;
    if (i < remaining) pip.classList.add('active');
    else if (i < 3) pip.classList.add('used');
    else pip.classList.add('empty');
  });
}

// ===== STATS UPDATE =====
function updateStats() {
  document.getElementById('entryCount').textContent = entryCount;
  document.getElementById('exitCount').textContent = exitCount;
  document.getElementById('peopleInside').textContent = peopleInside;
  document.getElementById('failedToday').textContent = failedToday;
}

// ===== VAULT STATE VISUAL =====
function setVaultOpen(isOpen, vaultNo) {
  vaultIsOpen = isOpen;
  const core = document.getElementById('vaultCore');
  const badge = document.getElementById('vaultBadge');
  const statusText = document.getElementById('vaultStatusText');
  const vaultNoEl = document.getElementById('vaultNo');

  if (isOpen) {
    core.classList.add('unlocked');
    badge.textContent = 'OPEN';
    badge.className = 'panel-badge open';
    statusText.textContent = 'OPEN';
    if (vaultNo) vaultNoEl.textContent = vaultNo;
  } else {
    core.classList.remove('unlocked');
    badge.textContent = 'SECURED';
    badge.className = 'panel-badge';
    statusText.textContent = 'LOCKED';
  }
}

// ===== POLLING SENSOR STATE =====
setInterval(async () => {
  try {
    const res = await fetch('/api/get_state');
    const data = await res.json();

    // Distance update
    if (Math.abs(data.distance - lastDist) > 4) {
      document.getElementById('distance').textContent = data.distance;
      updateArc(data.distance);

      if (data.distance < 300 && data.distance > 0 && lastDist !== -1) {
        addLog(`Motion detected — ${data.distance}cm from sensor`, 'motion');
      }
      lastDist = data.distance;
    }

    updateThreat(data.distance, data.ir_triggered);

    // Health: API is online
    document.getElementById('apiBar').style.width = '100%';
    document.getElementById('apiVal').textContent = 'ONLINE';

    // Trigger modal
    if (data.ir_triggered && !modalOpen) {
      openModal();
    }
  } catch (e) {
    document.getElementById('apiBar').style.width = '20%';
    document.getElementById('apiVal').textContent = 'ERROR';
    document.getElementById('connDot').style.background = '#ff3333';
    document.getElementById('connDot').style.boxShadow = '0 0 8px #ff3333';
  }
}, 500);

// ===== MODAL OPEN =====
function openModal() {
  const modal = document.getElementById('authModal');
  const box = document.getElementById('modalBox');
  box.className = 'modal-frame';
  box.innerHTML = getModalDefaultHTML();
  modal.classList.add('active');
  modalOpen = true;

  addLog('BIOMETRIC PROXIMITY TRIGGERED — awaiting credential input', 'alert');
  document.getElementById('sysStatus') && (document.getElementById('sysStatus').style.color = '#ff3333');

  setAttemptPips(0);

  // Focus first input
  setTimeout(() => document.getElementById('clientId')?.focus(), 100);
}

function getModalDefaultHTML() {
  return `
    <div class="modal-scan-bar"></div>
    <div class="modal-header-strip">
      <svg width="16" height="16" viewBox="0 0 20 20"><polygon points="10,1 19,5.5 19,14.5 10,19 1,14.5 1,5.5" fill="none" stroke="#ff3333" stroke-width="1"/></svg>
      <span>NEXUS SECURITY — IDENTITY VERIFICATION</span>
    </div>
    <div class="modal-body">
      <div class="biometric-icon">
        <svg width="64" height="64" viewBox="0 0 64 64">
          <circle cx="32" cy="20" r="10" fill="none" stroke="#ff3333" stroke-width="1.5"/>
          <path d="M12 56 Q12 40 32 40 Q52 40 52 56" fill="none" stroke="#ff3333" stroke-width="1.5"/>
          <circle cx="32" cy="20" r="3" fill="#ff3333" opacity="0.5"/>
          <path d="M18 8 Q32 2 46 8" fill="none" stroke="#ff3333" stroke-width="1" stroke-dasharray="3,2" opacity="0.6"/>
        </svg>
        <div class="biometric-pulse"></div>
      </div>
      <h2 class="modal-title">BIOMETRIC ALERT</h2>
      <p class="modal-subtitle">UNAUTHORIZED PROXIMITY DETECTED<br>CREDENTIAL VERIFICATION REQUIRED</p>
      <div class="input-group">
        <label class="input-label">CLIENT IDENTIFIER</label>
        <input type="text" id="clientId" placeholder="Enter your ID" autocomplete="off" class="auth-input">
      </div>
      <div class="input-group">
        <label class="input-label">SECURITY PASSPHRASE</label>
        <input type="password" id="clientPass" placeholder="Enter password" autocomplete="off" class="auth-input"
          onkeydown="if(event.key==='Enter')attemptLogin()">
      </div>
      <button onclick="attemptLogin()" id="authBtn" class="auth-btn">
        <span id="authBtnText">AUTHORIZE ACCESS</span>
      </button>
      <div id="statusMsg" class="status-msg"></div>
      <div class="attempt-indicator" id="attemptIndicator">
        <span class="attempt-pip active"></span>
        <span class="attempt-pip active"></span>
        <span class="attempt-pip active"></span>
      </div>
    </div>
  `;
}

// ===== LOGIN =====
async function attemptLogin() {
  const id = document.getElementById('clientId')?.value || '';
  const pass = document.getElementById('clientPass')?.value || '';
  const msgBox = document.getElementById('statusMsg');
  const btn = document.getElementById('authBtn');

  if (!id || !pass) {
    msgBox.textContent = 'ALL FIELDS REQUIRED.';
    return;
  }

  btn.disabled = true;
  document.getElementById('authBtnText').textContent = 'VERIFYING...';
  addLog(`Auth attempt initiated for ID: ${id.toUpperCase()}`, 'system');

  try {
    const res = await fetch('/api/login', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ id, password: pass })
    });
    const data = await res.json();

    if (data.success) {
      // Update counters
      entryCount++;
      peopleInside++;
      updateStats();

      // Timeline
      addTimelineEvent(id, data.vault_no, true);

      // Logs
      addLog(`ACCESS GRANTED — ${id.toUpperCase()} cleared for Vault ${data.vault_no}`, 'success');
      addLog(`Vault ${data.vault_no} door relay triggered via ESP32`, 'success');

      // Last opened time
      const now = new Date();
      document.getElementById('lastOpened').textContent =
        `${String(now.getHours()).padStart(2,'0')}:${String(now.getMinutes()).padStart(2,'0')}:${String(now.getSeconds()).padStart(2,'0')}`;

      // ESP32 health show as linked
      document.getElementById('esp32Bar').style.width = '92%';
      document.getElementById('esp32Val').textContent = 'LINKED';

      // Open vault visual
      setVaultOpen(true, data.vault_no);

      // Show success modal
      showSuccessModal(id, data.vault_no);

      // Auto close after 7s with countdown
      let remaining = 7;
      const interval = setInterval(() => {
        remaining--;
        const bar = document.getElementById('countdownBar');
        const label = document.getElementById('countdownLabel');
        if (bar) bar.style.width = ((remaining / 7) * 100) + '%';
        if (label) label.textContent = `SECURING IN ${remaining}s`;
        if (remaining <= 0) {
          clearInterval(interval);
          closeModal();
          // After person exits vault, count them out
          setTimeout(() => {
            exitCount++;
            peopleInside = Math.max(0, peopleInside - 1);
            setVaultOpen(false);
            addLog(`Vault secured. Session ended.`, 'system');
            updateStats();
          }, 3000);
        }
      }, 1000);

    } else {
      // Count failed attempts to extract number remaining
      const match = data.error.match(/(\d+) attempts? left/);
      const attemptsLeft = match ? parseInt(match[1]) : 0;
      const attemptsUsed = 3 - attemptsLeft;
      failedToday++;
      updateStats();
      setAttemptPips(attemptsUsed);
      addTimelineEvent(id, '---', false);
      addLog(`AUTH FAILED: ${data.error}`, 'alert');

      msgBox.textContent = data.error;
      btn.disabled = false;
      document.getElementById('authBtnText').textContent = 'AUTHORIZE ACCESS';
      document.getElementById('clientPass').value = '';
      document.getElementById('clientPass').focus();

      // Shake animation
      const box = document.getElementById('modalBox');
      box.style.animation = 'none';
      box.offsetHeight;
      box.style.animation = 'shake 0.4s ease';
    }
  } catch (e) {
    msgBox.textContent = 'SERVER CONNECTION ERROR.';
    btn.disabled = false;
    document.getElementById('authBtnText').textContent = 'RETRY';
  }
}

function showSuccessModal(userId, vaultNo) {
  const box = document.getElementById('modalBox');
  box.className = 'modal-frame success-state';
  box.innerHTML = `
    <div class="modal-scan-bar"></div>
    <div class="modal-header-strip" style="border-color:rgba(0,204,106,0.2);color:var(--green);">
      <svg width="16" height="16" viewBox="0 0 20 20"><polygon points="10,1 19,5.5 19,14.5 10,19 1,14.5 1,5.5" fill="none" stroke="#00cc6a" stroke-width="1"/></svg>
      <span>NEXUS SECURITY — ACCESS AUTHORIZED</span>
    </div>
    <div class="modal-body">
      <div class="success-content">
        <div class="success-checkmark">
          <svg width="32" height="32" viewBox="0 0 32 32">
            <polyline points="6,17 13,24 26,9" fill="none" stroke="#00cc6a" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"/>
          </svg>
        </div>
        <div class="success-title">ACCESS GRANTED</div>
        <div class="success-sub">IDENTITY VERIFIED · VAULT UNLOCKED</div>
        <div style="display:flex;flex-direction:column;align-items:center;gap:4px;margin:16px 0;padding:14px;background:rgba(201,168,76,0.06);border:1px solid rgba(201,168,76,0.2);border-radius:4px;">
          <div style="font-family:var(--mono);font-size:0.58rem;color:var(--text-dim);letter-spacing:2px;">AUTHORIZED CLIENT</div>
          <div style="font-family:var(--display);font-size:1.1rem;font-weight:700;color:var(--text);letter-spacing:3px;">${userId.toUpperCase()}</div>
          <div style="font-family:var(--mono);font-size:0.58rem;color:var(--text-dim);letter-spacing:2px;margin-top:8px;">VAULT NUMBER</div>
          <div class="success-vault-no">${vaultNo}</div>
        </div>
        <div class="countdown-bar-wrap">
          <div class="countdown-bar" id="countdownBar" style="transition: width 1s linear;"></div>
        </div>
        <div class="countdown-label" id="countdownLabel">SECURING IN 7s</div>
      </div>
    </div>
  `;
}

function closeModal() {
  document.getElementById('authModal').classList.remove('active');
  modalOpen = false;
  updateThreat(lastDist, false);
}

// Add shake keyframe dynamically
const style = document.createElement('style');
style.textContent = `@keyframes shake {
  0%,100%{transform:translateX(0)}
  15%{transform:translateX(-8px)}
  30%{transform:translateX(8px)}
  45%{transform:translateX(-6px)}
  60%{transform:translateX(6px)}
  75%{transform:translateX(-4px)}
  90%{transform:translateX(4px)}
}`;
document.head.appendChild(style);