// Panel del profesor

async function cargarNivelesPanel() {
  try {
    const res = await fetch('/api/niveles');
    const niveles = await res.json();
    const select = document.getElementById('filtro-nivel');
    niveles.forEach(n => {
      const opt = document.createElement('option');
      opt.value = n.numero;
      opt.textContent = `Nivel ${n.numero}: ${n.titulo}`;
      select.appendChild(opt);
    });
  } catch {}
}

async function filtrar() {
  const seccion = document.getElementById('filtro-seccion').value;
  const nivel = document.getElementById('filtro-nivel').value;
  const estado = document.getElementById('filtro-estado').value;
  const alertBox = document.getElementById('alert-box');

  const params = new URLSearchParams();
  if (seccion) params.set('seccion', seccion);
  if (nivel) params.set('nivel', nivel);
  if (estado) params.set('estado', estado);

  try {
    const res = await fetch(`/api/panel/entregas?${params}`, { headers: authHeaders() });
    if (!res.ok) throw new Error();
    const entregas = await res.json();
    renderEntregas(entregas);
  } catch {
    alertBox.innerHTML = '<div class="alert alert-error">Error cargando entregas</div>';
  }
}

function renderEntregas(entregas) {
  const container = document.getElementById('entregas-list');
  if (entregas.length === 0) {
    container.innerHTML = '<p style="color:#94a3b8;">No hay entregas con esos filtros.</p>';
    return;
  }

  container.innerHTML = `
    <table class="entregas-table">
      <thead>
        <tr>
          <th><input type="checkbox" onchange="toggleAll(this)"></th>
          <th>Alumno</th>
          <th>Seccion</th>
          <th>Nivel</th>
          <th>Estado</th>
          <th>Fecha</th>
          <th>Acciones</th>
        </tr>
      </thead>
      <tbody>
        ${entregas.map(e => `
          <tr id="row-${e.id}">
            <td><input type="checkbox" class="chk-entrega" value="${e.id}"></td>
            <td>${escapeHtml(e.nombre)}</td>
            <td>${e.seccion || '-'}</td>
            <td>Nivel ${e.nivel_numero}</td>
            <td><span class="badge badge-${e.estado}">${e.estado}</span></td>
            <td>${new Date(e.created_at).toLocaleString('es-AR')}</td>
            <td>
              <a href="/p/${e.id}" target="_blank">Ver</a>
              <button class="btn btn-success btn-sm" onclick="corregir('${e.id}','aprobado')">OK</button>
              <button class="btn btn-danger btn-sm" onclick="corregir('${e.id}','rechazado')">X</button>
            </td>
          </tr>
        `).join('')}
      </tbody>
    </table>
  `;
}

function toggleAll(master) {
  document.querySelectorAll('.chk-entrega').forEach(c => c.checked = master.checked);
}

async function corregir(id, estado) {
  const nota = prompt('Nota para el alumno (opcional):') || '';
  const alertBox = document.getElementById('alert-box');

  try {
    const res = await fetch(`/api/panel/entregas/${id}`, {
      method: 'PATCH',
      headers: authHeaders(),
      body: JSON.stringify({ estado, nota_profe: nota })
    });
    if (!res.ok) throw new Error();
    alertBox.innerHTML = `<div class="alert alert-success">Entrega ${estado}</div>`;
    filtrar();
  } catch {
    alertBox.innerHTML = '<div class="alert alert-error">Error al corregir</div>';
  }
}

async function corregirLote(estado) {
  const checks = document.querySelectorAll('.chk-entrega:checked');
  if (checks.length === 0) return alert('Selecciona al menos una entrega');

  const nota = prompt('Nota para todas (opcional):') || '';
  const items = Array.from(checks).map(c => ({
    id: c.value, estado, nota_profe: nota
  }));
  const alertBox = document.getElementById('alert-box');

  try {
    const res = await fetch('/api/panel/corregir-lote', {
      method: 'POST',
      headers: authHeaders(),
      body: JSON.stringify(items)
    });
    if (!res.ok) throw new Error();
    const data = await res.json();
    alertBox.innerHTML = `<div class="alert alert-success">${data.actualizados} entregas actualizadas</div>`;
    filtrar();
  } catch {
    alertBox.innerHTML = '<div class="alert alert-error">Error en correccion por lote</div>';
  }
}

function escapeHtml(text) {
  const div = document.createElement('div');
  div.textContent = text;
  return div.innerHTML;
}
