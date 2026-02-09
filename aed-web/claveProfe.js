const crypto = require('crypto');
const nodemailer = require('nodemailer');

// Clave actual en memoria
let claveActual = generarClave();

function generarClave() {
  // 6 caracteres alfanumericos en mayuscula
  return crypto.randomBytes(4).toString('hex').toUpperCase().slice(0, 6);
}

function getClaveActual() {
  return claveActual;
}

async function enviarClave(clave) {
  const emailDestino = process.env.PROFE_EMAIL;
  if (!emailDestino) {
    console.log(`[CLAVE PROFE] No hay PROFE_EMAIL configurado. Clave actual: ${clave}`);
    return;
  }

  const smtpHost = process.env.SMTP_HOST || 'smtp.gmail.com';
  const smtpPort = parseInt(process.env.SMTP_PORT || '587', 10);
  const smtpUser = process.env.SMTP_USER;
  const smtpPass = process.env.SMTP_PASS;

  if (!smtpUser || !smtpPass) {
    console.log(`[CLAVE PROFE] SMTP no configurado. Clave actual: ${clave}`);
    return;
  }

  const transporter = nodemailer.createTransport({
    host: smtpHost,
    port: smtpPort,
    secure: smtpPort === 465,
    auth: { user: smtpUser, pass: smtpPass }
  });

  try {
    await transporter.sendMail({
      from: `"AED Pseudocodigo" <${smtpUser}>`,
      to: emailDestino,
      subject: 'Nueva clave de profesor - AED',
      text: `Tu nueva clave de registro de profesor es: ${clave}\n\nEsta clave expira en 15 minutos.`,
      html: `
        <div style="font-family:sans-serif; padding:20px; background:#282828; color:#ebdbb2; border-radius:8px;">
          <h2 style="color:#fabd2f;">AED Pseudocodigo</h2>
          <p>Tu nueva clave de registro de profesor es:</p>
          <h1 style="color:#fe8019; letter-spacing:0.2em; font-size:2rem;">${clave}</h1>
          <p style="color:#a89984; font-size:0.85rem;">Esta clave expira en 15 minutos.</p>
        </div>
      `
    });
    console.log(`[CLAVE PROFE] Clave enviada a ${emailDestino}`);
  } catch (err) {
    console.error(`[CLAVE PROFE] Error enviando email:`, err.message);
    console.log(`[CLAVE PROFE] Clave actual: ${clave}`);
  }
}

function iniciarRotacion() {
  // Enviar la primera clave al arrancar
  enviarClave(claveActual);

  // Rotar cada 15 minutos
  setInterval(() => {
    claveActual = generarClave();
    console.log(`[CLAVE PROFE] Clave rotada`);
    enviarClave(claveActual);
  }, 15 * 60 * 1000);
}

module.exports = { getClaveActual, iniciarRotacion };
