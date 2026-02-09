// Auth helpers
function getToken() { return localStorage.getItem('token'); }
function getUser() { return JSON.parse(localStorage.getItem('user') || 'null'); }

function authHeaders() {
  return { 'Content-Type': 'application/json', 'Authorization': 'Bearer ' + getToken() };
}

function requireAuth(role) {
  const token = getToken();
  const user = getUser();
  if (!token || !user) {
    window.location.href = '/';
    return;
  }
  if (role && user.role !== role) {
    window.location.href = '/';
  }
}

function updateNav() {
  const nav = document.getElementById('nav-links');
  if (!nav) return;
  const user = getUser();
  if (user) {
    if (user.role === 'profe') {
      nav.innerHTML = `
        <span style="color:#94a3b8;">${user.nombre}</span>
        <a href="/panel">Panel</a>
        <button onclick="logout()">Cerrar Sesion</button>
      `;
    } else {
      nav.innerHTML = `
        <span style="color:#94a3b8;">${user.nombre}</span>
        <a href="/niveles" onclick="event.preventDefault(); volverANiveles();">Niveles</a>
        <a href="#" onclick="event.preventDefault(); abrirCodigoActual();">Codigo</a>
        <span style="color:#475569; font-size:0.85rem; cursor:default;">Wiki (Proximamente)</span>
        <button onclick="logout()">Cerrar Sesion</button>
      `;
    }
  }
}

function logout() {
  localStorage.removeItem('token');
  localStorage.removeItem('user');
  window.location.href = '/';
}

// Toggle campo clave profe (index.html)
function toggleClaveProfe() {
  const role = document.getElementById('reg-role');
  const grupo = document.getElementById('clave-profe-group');
  if (!role || !grupo) return;
  if (role.value === 'profe') {
    grupo.classList.remove('hidden');
  } else {
    grupo.classList.add('hidden');
  }
}

// Tabs (index.html)
function showTab(tab) {
  document.querySelectorAll('.tab').forEach(t => t.classList.remove('active'));
  if (tab === 'login') {
    document.getElementById('login-form').classList.remove('hidden');
    document.getElementById('register-form').classList.add('hidden');
    document.querySelectorAll('.tab')[0].classList.add('active');
  } else {
    document.getElementById('login-form').classList.add('hidden');
    document.getElementById('register-form').classList.remove('hidden');
    document.querySelectorAll('.tab')[1].classList.add('active');
  }
}

async function login() {
  const email = document.getElementById('login-email').value;
  const password = document.getElementById('login-pass').value;
  const alertBox = document.getElementById('auth-alert');

  try {
    const res = await fetch('/api/login', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ email, password })
    });
    const data = await res.json();
    if (!res.ok) {
      alertBox.innerHTML = `<div class="alert alert-error">${data.error}</div>`;
      return;
    }
    localStorage.setItem('token', data.token);
    localStorage.setItem('user', JSON.stringify(data.user));
    window.location.href = data.user.role === 'profe' ? '/panel' : '/niveles';
  } catch {
    alertBox.innerHTML = '<div class="alert alert-error">Error de conexion</div>';
  }
}

async function register() {
  const nombre = document.getElementById('reg-nombre').value;
  const email = document.getElementById('reg-email').value;
  const password = document.getElementById('reg-pass').value;
  const seccion = document.getElementById('reg-seccion').value;
  const role = document.getElementById('reg-role').value;
  const claveProfeInput = document.getElementById('reg-clave-profe');
  const clave_profe = claveProfeInput ? claveProfeInput.value : '';
  const alertBox = document.getElementById('auth-alert');

  try {
    const res = await fetch('/api/register', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ email, password, nombre, seccion, role, clave_profe })
    });
    const data = await res.json();
    if (!res.ok) {
      alertBox.innerHTML = `<div class="alert alert-error">${data.error}</div>`;
      return;
    }
    localStorage.setItem('token', data.token);
    localStorage.setItem('user', JSON.stringify(data.user));
    window.location.href = data.user.role === 'profe' ? '/panel' : '/niveles';
  } catch {
    alertBox.innerHTML = '<div class="alert alert-error">Error de conexion</div>';
  }
}

// ========== IDE (niveles.html) ==========

let nivelesData = [];
let nivelSeleccionado = null;

async function initIDE() {
  await cargarNiveles();
  setupEditor();
}

async function cargarNiveles() {
  try {
    const res = await fetch('/api/niveles-progreso', { headers: authHeaders() });
    nivelesData = await res.json();
    renderNivelesGrid();
  } catch {
    const grid = document.getElementById('niveles-grid');
    if (grid) grid.innerHTML = '<div class="alert alert-error">Error cargando niveles</div>';
  }
}

function renderNivelesGrid() {
  const container = document.getElementById('niveles-grid');
  if (!container) return;
  container.innerHTML = nivelesData.map(n => {
    const locked = !n.desbloqueado;
    let cls = 'nivel-card-grid';
    if (locked) cls += ' locked';
    if (n.aprobado) cls += ' aprobado';
    const onclick = locked ? '' : `seleccionarNivel(${n.id})`;
    return `
      <div class="${cls}" onclick="${onclick}">
        <div class="nivel-card-header">
          <span class="nivel-card-numero">Nivel ${n.numero}</span>
          ${locked ? '<span class="nivel-lock">&#128274;</span>' : ''}
          ${n.aprobado ? '<span class="nivel-check">&#10003;</span>' : ''}
        </div>
        <h3>${n.titulo}</h3>
        <p>${n.enunciado.substring(0, 100)}...</p>
        ${locked ? '<span class="nivel-locked-msg">Aproba el nivel anterior para desbloquear</span>' : '<span class="nivel-open-msg">Click para abrir</span>'}
      </div>
    `;
  }).join('');
}

function mostrarVista(vista) {
  const vistaNiveles = document.getElementById('vista-niveles');
  const vistaIde = document.getElementById('vista-ide');
  if (vista === 'niveles') {
    vistaNiveles.classList.remove('hidden');
    vistaIde.classList.add('hidden');
  } else {
    vistaNiveles.classList.add('hidden');
    vistaIde.classList.remove('hidden');
  }
}

function volverANiveles() {
  renderNivelesGrid();
  mostrarVista('niveles');
}

function abrirCodigoActual() {
  if (nivelSeleccionado) {
    mostrarVista('ide');
  } else {
    // Si no hay nivel seleccionado, seleccionar el primero desbloqueado
    const primero = nivelesData.find(n => n.desbloqueado && !n.aprobado) || nivelesData.find(n => n.desbloqueado);
    if (primero) seleccionarNivel(primero.id);
  }
}

function seleccionarNivel(nivelId) {
  const nivel = nivelesData.find(n => n.id === nivelId);
  if (!nivel || !nivel.desbloqueado) return;
  nivelSeleccionado = nivel;

  // Mostrar IDE
  mostrarVista('ide');

  // Panel izquierdo
  document.getElementById('editor-titulo').textContent = `Nivel ${nivel.numero}: ${nivel.titulo}`;
  document.getElementById('editor-enunciado').textContent = nivel.enunciado;

  const pistaBtn = document.getElementById('pista-btn');
  const pistaContent = document.getElementById('editor-pista');
  if (nivel.pista) {
    pistaBtn.classList.remove('hidden');
    pistaContent.textContent = nivel.pista;
    pistaContent.classList.add('hidden');
  } else {
    pistaBtn.classList.add('hidden');
    pistaContent.classList.add('hidden');
  }

  // Toolbar
  document.getElementById('editor-nivel-info').textContent = `Nivel ${nivel.numero}: ${nivel.titulo}`;
  document.getElementById('btn-enviar').disabled = false;

  // Limpiar editor y errores
  document.getElementById('editor-content').value = '';
  clearErrors();
  updateLineNumbers();

  // Cargar entregas de este nivel
  cargarEntregasNivel(nivelId);
}

function togglePista() {
  document.getElementById('editor-pista').classList.toggle('hidden');
}

// Editor con numeros de linea
function setupEditor() {
  const textarea = document.getElementById('editor-content');
  const lineNumbers = document.getElementById('line-numbers');

  textarea.addEventListener('input', updateLineNumbers);
  textarea.addEventListener('scroll', () => {
    lineNumbers.scrollTop = textarea.scrollTop;
  });

  updateLineNumbers();
}

function updateLineNumbers() {
  const textarea = document.getElementById('editor-content');
  const lineNumbers = document.getElementById('line-numbers');
  const lines = textarea.value.split('\n');
  const count = Math.max(lines.length, 1);

  lineNumbers.innerHTML = '';
  for (let i = 1; i <= count; i++) {
    const span = document.createElement('span');
    span.className = 'line-num';
    span.textContent = i;
    span.dataset.line = i;
    lineNumbers.appendChild(span);
  }
}

// Enviar paste
async function enviarPaste() {
  if (!nivelSeleccionado) return;
  const content = document.getElementById('editor-content').value.trim();
  const alertBox = document.getElementById('alert-box');

  if (!content) {
    alertBox.innerHTML = '<div class="alert alert-error">Escribi tu pseudocodigo</div>';
    return;
  }

  clearErrors();
  alertBox.innerHTML = '';

  try {
    const res = await fetch('/api/pastes', {
      method: 'POST',
      headers: authHeaders(),
      body: JSON.stringify({ nivel_id: nivelSeleccionado.id, content })
    });
    const data = await res.json();
    if (!res.ok) {
      if (data.errores && Array.isArray(data.errores)) {
        displayInlineErrors(data.errores);
      } else {
        alertBox.innerHTML = `<div class="alert alert-error">${data.error}</div>`;
      }
      return;
    }
    alertBox.innerHTML = `<div class="alert alert-success">Entrega enviada! <a href="${data.url}">Ver entrega</a></div>`;
    cargarEntregasNivel(nivelSeleccionado.id);
    // Recargar niveles por si se desbloqueo algo
    cargarNiveles();
  } catch {
    alertBox.innerHTML = '<div class="alert alert-error">Error enviando</div>';
  }
}

function displayInlineErrors(errores) {
  // Marcar lineas con error en el gutter
  const lineNums = document.querySelectorAll('#line-numbers .line-num');
  const errorLines = new Set();

  errores.forEach(e => {
    if (e.line > 0) errorLines.add(e.line);
  });

  lineNums.forEach(span => {
    const num = parseInt(span.dataset.line, 10);
    if (errorLines.has(num)) {
      span.classList.add('has-error');
    }
  });

  // Panel de errores abajo
  const panel = document.getElementById('error-summary');
  panel.classList.remove('hidden');
  panel.innerHTML = errores.map(e => {
    if (e.line > 0) {
      return `<div class="error-item"><span class="error-line">Linea ${e.line}:</span> ${escapeHtml(e.message)}</div>`;
    }
    return `<div class="error-item"><span class="error-general">General:</span> ${escapeHtml(e.message)}</div>`;
  }).join('');
}

function clearErrors() {
  document.querySelectorAll('#line-numbers .line-num.has-error').forEach(el => {
    el.classList.remove('has-error');
  });
  const panel = document.getElementById('error-summary');
  if (panel) {
    panel.classList.add('hidden');
    panel.innerHTML = '';
  }
}

function escapeHtml(text) {
  const div = document.createElement('div');
  div.textContent = text;
  return div.innerHTML;
}

// Entregas del nivel seleccionado
async function cargarEntregasNivel(nivelId) {
  const container = document.getElementById('entregas-nivel');
  if (!container) return;

  try {
    const res = await fetch('/api/mis-pastes', { headers: authHeaders() });
    if (!res.ok) return;
    const todas = await res.json();
    const entregas = todas.filter(e => e.nivel_id === nivelId);

    if (entregas.length === 0) {
      container.innerHTML = '<p style="color:#64748b; font-size:0.85rem;">Sin entregas aun.</p>';
      return;
    }

    container.innerHTML = entregas.map(e => `
      <div class="card" style="padding:0.75rem; margin-bottom:0.5rem;">
        <div style="display:flex; justify-content:space-between; align-items:center;">
          <a href="/p/${e.id}" style="font-size:0.85rem;">Entrega</a>
          <span class="badge badge-${e.estado}">${e.estado}</span>
        </div>
        <div style="font-size:0.75rem; color:#64748b;">${new Date(e.created_at).toLocaleString('es-AR')}</div>
        ${e.nota_profe ? `<div class="nota-profe" style="margin-top:0.3rem; font-size:0.8rem;">${escapeHtml(e.nota_profe)}</div>` : ''}
      </div>
    `).join('');
  } catch {
    container.innerHTML = '<p style="color:#fca5a5; font-size:0.85rem;">Error cargando entregas</p>';
  }
}

// Legacy functions for backwards compat (used by old pages if still referenced)
async function cargarNiveles_legacy() {}
async function cargarMisEntregas() {
  const container = document.getElementById('mis-entregas');
  if (!container) return;

  try {
    const res = await fetch('/api/mis-pastes', { headers: authHeaders() });
    if (!res.ok) return;
    const entregas = await res.json();

    if (entregas.length === 0) {
      container.innerHTML = '<p style="color:#94a3b8;">Aun no enviaste entregas.</p>';
      return;
    }

    container.innerHTML = entregas.map(e => `
      <div class="card">
        <div style="display:flex; justify-content:space-between; align-items:center;">
          <h3>Nivel ${e.nivel_numero}: ${e.nivel_titulo}</h3>
          <span class="badge badge-${e.estado}">${e.estado}</span>
        </div>
        <div class="paste-meta">
          <span>${new Date(e.created_at).toLocaleString('es-AR')}</span>
          <a href="/p/${e.id}">Ver entrega</a>
        </div>
        ${e.nota_profe ? `<div class="nota-profe"><strong>Nota del profe:</strong> ${e.nota_profe}</div>` : ''}
      </div>
    `).join('');
  } catch {
    container.innerHTML = '<div class="alert alert-error">Error cargando entregas</div>';
  }
}
