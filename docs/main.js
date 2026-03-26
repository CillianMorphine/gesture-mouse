/* main.js — GestureMouse landing */
'use strict';

// ════════════════════════════════════════════════
// PARTICLE CANVAS
// ════════════════════════════════════════════════
(function initParticles() {
  const canvas = document.getElementById('particle-canvas');
  if (!canvas) return;
  const ctx = canvas.getContext('2d');

  let W, H, particles = [];
  const COUNT = 60;
  const ACCENT = 'rgba(74,158,255,';
  const ACCENT2 = 'rgba(0,229,195,';

  function resize() {
    W = canvas.width  = canvas.offsetWidth;
    H = canvas.height = canvas.offsetHeight;
  }

  function rand(min, max) { return Math.random() * (max - min) + min; }

  function createParticle() {
    return {
      x: rand(0, W), y: rand(0, H),
      vx: rand(-0.3, 0.3), vy: rand(-0.3, 0.3),
      r: rand(1, 3),
      color: Math.random() > 0.7 ? ACCENT2 : ACCENT,
      opacity: rand(0.2, 0.7),
    };
  }

  function init() {
    resize();
    particles = Array.from({ length: COUNT }, createParticle);
  }

  function drawLine(a, b) {
    const dist = Math.hypot(a.x - b.x, a.y - b.y);
    if (dist > 120) return;
    const alpha = (1 - dist / 120) * 0.25;
    ctx.beginPath();
    ctx.moveTo(a.x, a.y);
    ctx.lineTo(b.x, b.y);
    ctx.strokeStyle = `rgba(74,158,255,${alpha})`;
    ctx.lineWidth = 0.8;
    ctx.stroke();
  }

  function tick() {
    ctx.clearRect(0, 0, W, H);

    particles.forEach(p => {
      p.x += p.vx; p.y += p.vy;
      if (p.x < 0) p.x = W; if (p.x > W) p.x = 0;
      if (p.y < 0) p.y = H; if (p.y > H) p.y = 0;

      // draw dot
      ctx.beginPath();
      ctx.arc(p.x, p.y, p.r, 0, Math.PI * 2);
      ctx.fillStyle = `${p.color}${p.opacity})`;
      ctx.fill();
    });

    // draw connections
    for (let i = 0; i < particles.length; i++) {
      for (let j = i + 1; j < particles.length; j++) {
        drawLine(particles[i], particles[j]);
      }
    }

    requestAnimationFrame(tick);
  }

  window.addEventListener('resize', resize, { passive: true });
  init();
  tick();
})();

// ════════════════════════════════════════════════
// MOBILE NAV TOGGLE
// ════════════════════════════════════════════════
(function initNav() {
  const toggle  = document.querySelector('.nav__toggle');
  const list    = document.querySelector('.nav__list');
  if (!toggle || !list) return;

  toggle.addEventListener('click', () => {
    const expanded = toggle.getAttribute('aria-expanded') === 'true';
    toggle.setAttribute('aria-expanded', String(!expanded));
    list.classList.toggle('is-open', !expanded);
  });

  // Close nav when a link is clicked
  list.addEventListener('click', e => {
    if (e.target.closest('.nav__link')) {
      toggle.setAttribute('aria-expanded', 'false');
      list.classList.remove('is-open');
    }
  });

  // Close on outside click
  document.addEventListener('click', e => {
    if (!e.target.closest('.nav')) {
      toggle.setAttribute('aria-expanded', 'false');
      list.classList.remove('is-open');
    }
  });
})();

// ════════════════════════════════════════════════
// ACTIVE NAV LINK ON SCROLL
// ════════════════════════════════════════════════
(function initScrollSpy() {
  const links    = document.querySelectorAll('.nav__link[href^="#"]');
  const sections = [];

  links.forEach(link => {
    const id = link.getAttribute('href').slice(1);
    const el = document.getElementById(id);
    if (el) sections.push({ link, el });
  });

  if (!sections.length) return;

  const observer = new IntersectionObserver(entries => {
    entries.forEach(entry => {
      const match = sections.find(s => s.el === entry.target);
      if (match) match.link.classList.toggle('nav__link--active', entry.isIntersecting);
    });
  }, { rootMargin: '-40% 0px -55% 0px' });

  sections.forEach(s => observer.observe(s.el));
})();

// ════════════════════════════════════════════════
// FADE-IN ON SCROLL (intersection observer)
// ════════════════════════════════════════════════
(function initFadeIn() {
  const targets = document.querySelectorAll(
    '.stats__item, .tasks-list__item, .pipeline__step, ' +
    '.gesture-card, .result-card, .section__header'
  );

  if (!targets.length) return;

  // Add base class
  targets.forEach((el, i) => {
    el.style.opacity = '0';
    el.style.transform = 'translateY(16px)';
    el.style.transition = `opacity 0.5s ease ${i % 6 * 0.07}s, transform 0.5s ease ${i % 6 * 0.07}s`;
  });

  const observer = new IntersectionObserver(entries => {
    entries.forEach(entry => {
      if (entry.isIntersecting) {
        entry.target.style.opacity = '1';
        entry.target.style.transform = 'translateY(0)';
        observer.unobserve(entry.target);
      }
    });
  }, { threshold: 0.12 });

  targets.forEach(el => observer.observe(el));
})();

// ════════════════════════════════════════════════
// STICKY HEADER — add shadow on scroll
// ════════════════════════════════════════════════
(function initStickyHeader() {
  const header = document.querySelector('.site-header');
  if (!header) return;

  const handler = () => {
    header.style.boxShadow = window.scrollY > 10
      ? '0 4px 24px rgba(0,0,0,.5)'
      : 'none';
  };

  window.addEventListener('scroll', handler, { passive: true });
})();
