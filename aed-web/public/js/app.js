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
    nav.innerHTML = `
      <span style="color:#94a3b8;">${user.nombre}</span>
      ${user.role === 'profe' ? '<a href="/panel">Panel</a>' : '<a href="/niveles">Niveles</a>'}
      <button onclick="logout()">Salir</button>
    `;
  }
}

function logout() {
  localStorage.removeItem('token');
  localStorage.removeItem('user');
  window.location.href = '/';
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
  const alertBox = document.getElementById('auth-alert');

  try {
    const res = await fetch('/api/register', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ email, password, nombre, seccion, role })
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

// Niveles (niveles.html)
let nivelesData = [];
let nivelSeleccionado = null;

async function cargarNiveles() {
  try {
    const res = await fetch('/api/niveles');
    nivelesData = await res.json();
    const container = document.getElementById('niveles-list');
    container.innerHTML = nivelesData.map(n => `
      <div class="card nivel-card" onclick="abrirEditor(${n.id})">
        <h3>Nivel ${n.numero}: ${n.titulo}</h3>
        <p style="color:#94a3b8; font-size:0.85rem;">${n.enunciado.substring(0, 120)}...</p>
      </div>
    `).join('');
  } catch {
    document.getElementById('niveles-list').innerHTML =
      '<div class="alert alert-error">Error cargando niveles</div>';
  }
}

function abrirEditor(nivelId) {
  const nivel = nivelesData.find(n => n.id === nivelId);
  if (!nivel) return;
  nivelSeleccionado = nivel;
  document.getElementById('editor-titulo').textContent = `Nivel ${nivel.numero}: ${nivel.titulo}`;
  document.getElementById('editor-enunciado').textContent = nivel.enunciado;
  document.getElementById('editor-pista').textContent = nivel.pista || '';
  document.getElementById('editor-pista').classList.add('hidden');
  document.getElementById('editor-content').value = '';
  document.getElementById('editor-section').classList.remove('hidden');
  document.getElementById('editor-section').scrollIntoView({ behavior: 'smooth' });
}

function cerrarEditor() {
  document.getElementById('editor-section').classList.add('hidden');
  nivelSeleccionado = null;
}

function togglePista() {
  document.getElementById('editor-pista').classList.toggle('hidden');
}

async function enviarPaste() {
  if (!nivelSeleccionado) return;
  const content = document.getElementById('editor-content').value.trim();
  const alertBox = document.getElementById('alert-box');

  if (!content) {
    alertBox.innerHTML = '<div class="alert alert-error">Escribi tu pseudocodigo</div>';
    return;
  }

  try {
    const res = await fetch('/api/pastes', {
      method: 'POST',
      headers: authHeaders(),
      body: JSON.stringify({ nivel_id: nivelSeleccionado.id, content })
    });
    const data = await res.json();
    if (!res.ok) {
      alertBox.innerHTML = `<div class="alert alert-error">${data.error}</div>`;
      return;
    }
    alertBox.innerHTML = `<div class="alert alert-success">Entrega enviada! <a href="${data.url}">${data.url}</a></div>`;
    cerrarEditor();
    cargarMisEntregas();
  } catch {
    alertBox.innerHTML = '<div class="alert alert-error">Error enviando</div>';
  }
}

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
