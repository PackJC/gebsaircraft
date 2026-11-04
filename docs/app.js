// Geb's Aircraft's site script: the Geb's family navigation, scroll reveals and copy toast (as Alien Invasion's site
// has them), plus this site's own drifting clouds in the hero, the switches between planes and install paths, XML
// colouring and the classname table.
(() => {
  "use strict";
  const $ = (s, r = document) => r.querySelector(s);
  const $$ = (s, r = document) => [...r.querySelectorAll(s)];
  document.documentElement.classList.add("js");
  const reducedMotion = window.matchMedia("(prefers-reduced-motion: reduce)").matches;

  // The Steam Workshop page, once the mod is published there. Empty: every Workshop link stays hidden.
  const WORKSHOP = "";
  $$("[data-workshop]").forEach(a => { if (WORKSHOP) a.href = WORKSHOP; else a.hidden = true; });

  const classnames = [
    { name: "GebsPA18_Yellow", type: "Aircraft", img: "pa18", description: "Piper PA-18 Super Cub in Cub Yellow, with its black lightning stripe. Two seats, flown realistically." },
    { name: "GebsPA18_Winter", type: "Aircraft", img: "pa18", description: "The Super Cub in winter camouflage: white with grey patches." },
    { name: "GebsPA18_Desert", type: "Aircraft", img: "pa18", description: "The Super Cub in desert camouflage: sand with brown patches." },
    { name: "GebsPA18F_Yellow", type: "Floatplane", img: "pa18f", description: "The Super Cub on floats, flown arcade: the easy plane. Spawn it on the water." },
    { name: "GebsPA18F_Winter", type: "Floatplane", img: "pa18f", description: "The Super Cub on floats in winter camouflage, its floats painted to match." },
    { name: "GebsPA18F_Desert", type: "Floatplane", img: "pa18f_desert", description: "The Super Cub on floats in desert camouflage, its floats painted to match." },
    { name: "GebsBF109E3", type: "Aircraft", img: "bf109e3", description: "Messerschmitt Bf 109 E-3 in its 1940 Channel-front scheme: retractable gear, flaps, slats, a working cockpit." },
    { name: "GebsBF109E3_Winter", type: "Aircraft", img: "bf109e3_winter", description: "The Bf 109 E-3 whitewashed for the winter, its markings and yellow theatre colours left." },
    { name: "GebsBF109E3_Desert", type: "Aircraft", img: "bf109e3", description: "The Bf 109 E-3 in a 1941 tropical scheme: sand with olive mottles, a white fuselage band and wingtips." },
    { name: "GebsBF109W", type: "Floatplane", img: "bf109w", description: "The Bf 109 W: the E-3 on twin floats, its gear folded away. Spawn it on the water." },
    { name: "GebsBF109W_Winter", type: "Floatplane", img: "bf109w", description: "The Bf 109 W whitewashed for the winter, its floats too." },
    { name: "GebsBF109W_Desert", type: "Floatplane", img: "bf109w", description: "The Bf 109 W in the tropical scheme, its floats sand and olive over light blue." },
    { name: "GebsPA18_Wheel", type: "Wheel", description: "The Super Cub's main wheel: a low-pressure bush tyre. Spawns as industrial loot." },
    { name: "GebsBF109_Wheel", type: "Wheel", description: "The Bf 109's 650 x 150 mm main wheel. Spawns as industrial loot." }
  ];

  // ---- copy + toast
  const toast = $("#toast");
  let toastTimer;
  async function copy(text, label = "Copied to clipboard") {
    try { await navigator.clipboard.writeText(text); }
    catch {
      const t = document.createElement("textarea");
      t.value = text;
      t.style.cssText = "position:fixed;opacity:0";
      document.body.append(t); t.select(); document.execCommand("copy"); t.remove();
    }
    toast.textContent = label;
    toast.classList.add("show");
    clearTimeout(toastTimer);
    toastTimer = setTimeout(() => toast.classList.remove("show"), 1500);
  }
  document.addEventListener("click", e => {
    const block = e.target.closest("[data-copy]");
    if (block) copy($("#" + block.dataset.copy).textContent.trim());
    const cls = e.target.closest("[data-cls]");
    if (cls) copy(cls.dataset.cls, cls.dataset.cls + " copied");
  });

  // ---- navigation: the menu on small screens, and the current section lit up in the bar
  const menu = $("#menu"), links = $("#nav-links");
  menu.addEventListener("click", () => {
    const open = links.classList.toggle("open");
    menu.setAttribute("aria-expanded", String(open));
  });
  $$("a", links).forEach(a => a.addEventListener("click", () => {
    links.classList.remove("open"); menu.setAttribute("aria-expanded", "false");
  }));
  const navMap = new Map($$("a[href^='#']", links).map(a => [a.getAttribute("href").slice(1), a]));
  const spy = new IntersectionObserver(entries => {
    for (const en of entries) {
      if (!en.isIntersecting) continue;
      navMap.forEach(a => a.classList.remove("active"));
      const a = navMap.get(en.target.id);
      if (a) a.classList.add("active");
    }
  }, { rootMargin: "-45% 0px -50% 0px" });
  $$("main > section[id]").forEach(s => spy.observe(s));

  // ---- reveal on scroll
  const reveal = new IntersectionObserver(entries => {
    for (const en of entries) if (en.isIntersecting) { en.target.classList.add("in"); reveal.unobserve(en.target); }
  }, { rootMargin: "0px 0px -8% 0px" });
  const watch = root => $$(".reveal:not(.in)", root).forEach(el => reveal.observe(el));
  $$(".grid.items, .tiles").forEach(grid => [...grid.children].forEach((c, i) => {
    c.style.transitionDelay = Math.min(i % 4, 3) * 60 + "ms";
  }));
  watch(document);

  // ---- switches: which plane to read about flying, player or server owner. A button data-show="group:name"
  // shows the panel data-panel="group:name" and hides the rest of its group
  const switches = $$("[data-show]");
  switches.forEach(button => button.addEventListener("click", () => {
    const [group] = button.dataset.show.split(":");
    switches.filter(b => b.dataset.show.startsWith(group + ":"))
      .forEach(b => b.setAttribute("aria-pressed", String(b === button)));
    $$(`[data-panel^="${group}:"]`).forEach(panel => {
      panel.hidden = panel.dataset.panel !== button.dataset.show;
      watch(panel);
    });
  }));

  // ---- light XML colouring for code blocks marked .language-xml (copy reads textContent, which the spans keep)
  const escapeHtml = text => text.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;");
  function highlightXml(code) {
    const source = code.textContent;
    const tokenPattern = /(<!--[\s\S]*?-->)|(<\/?)([\w:.-]+)((?:\s+[\w:.-]+(?:="[^"]*")?)*)(\s*\/?>)/g;
    const span = (cls, text) => `<span class="${cls}">${escapeHtml(text)}</span>`;
    let html = "", last = 0, match;
    while ((match = tokenPattern.exec(source))) {
      html += escapeHtml(source.slice(last, match.index));
      if (match[1]) {
        html += span("tok-com", match[1]);
      } else {
        const attributes = match[4].replace(/([\w:.-]+)(="[^"]*")?/g, (_, name, value) =>
          span("tok-attr", name) + (value ? span("tok-punct", "=") + span("tok-val", value.slice(1)) : ""));
        html += span("tok-punct", match[2]) + span("tok-tag", match[3]) + attributes + span("tok-punct", match[5]);
      }
      last = tokenPattern.lastIndex;
    }
    code.innerHTML = html + escapeHtml(source.slice(last));
  }
  $$("code.language-xml").forEach(highlightXml);

  // ---- classnames
  const body = $("#classname-body"), search = $("#classname-search"), filter = $("#classname-filter");
  const count = $("#result-count"), empty = $("#empty-state"), table = $("#classname-table");
  function renderClassnames() {
    const query = search.value.trim().toLowerCase();
    const type = filter.value;
    const results = classnames.filter(item =>
      `${item.name} ${item.type} ${item.description}`.toLowerCase().includes(query) && (type === "all" || item.type === type));

    body.replaceChildren(...results.map(item => {
      const row = document.createElement("tr");
      const name = document.createElement("td");
      const cell = document.createElement("div");
      cell.className = "name-cell";
      const thumb = document.createElement("span");
      thumb.className = "thumb";
      if (item.img) {
        const img = document.createElement("img");
        img.src = `assets/${item.img}.webp`;
        img.alt = "";
        img.loading = "lazy";
        thumb.append(img);
      }
      const button = document.createElement("button");
      button.className = "cls";
      button.type = "button";
      button.title = "Copy classname";
      button.dataset.cls = item.name;
      button.textContent = item.name;
      cell.append(thumb, button);
      name.append(cell);
      const kind = document.createElement("td");
      const chip = document.createElement("span");
      chip.className = "chip";
      chip.textContent = item.type;
      kind.append(chip);
      const description = document.createElement("td");
      description.textContent = item.description;
      row.append(name, kind, description);
      return row;
    }));

    count.textContent = `${results.length} ${results.length === 1 ? "classname" : "classnames"}`;
    empty.hidden = results.length !== 0;
    table.hidden = results.length === 0;
  }
  search.addEventListener("input", renderClassnames);
  filter.addEventListener("change", renderClassnames);
  renderClassnames();

  // ---- the hero's sky: soft clouds drifting past at three depths, the far ones slower and fainter
  const sky = $("#clouds");
  if (sky && !reducedMotion) {
    const ctx = sky.getContext("2d");
    let clouds = [], width = 0, height = 0, visible = true, last = performance.now();

    function cloud(anywhere) {
      const depth = Math.random();
      return {
        x: anywhere ? Math.random() * width : width + 200,
        y: height * (0.08 + 0.75 * Math.random()),
        size: 60 + 160 * depth,
        speed: 6 + 22 * depth,
        alpha: 0.03 + 0.07 * depth,
        puffs: Array.from({ length: 5 }, () => [Math.random() * 1.8 - 0.9, Math.random() * 0.4 - 0.2, 0.45 + Math.random() * 0.5])
      };
    }

    function resize() {
      const dpr = Math.min(window.devicePixelRatio || 1, 2);
      width = sky.clientWidth;
      height = sky.clientHeight;
      sky.width = width * dpr;
      sky.height = height * dpr;
      ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
      const n = Math.max(6, Math.min(16, Math.round(width / 120)));
      clouds = Array.from({ length: n }, () => cloud(true));
    }

    function draw(now) {
      const dt = Math.min((now - last) / 1000, 0.1);
      last = now;
      if (visible) {
        ctx.clearRect(0, 0, width, height);
        for (let i = 0; i < clouds.length; i++) {
          const c = clouds[i];
          c.x -= c.speed * dt;
          if (c.x < -c.size * 2.5) clouds[i] = cloud(false);
          for (const [px, py, r] of c.puffs) {
            const x = c.x + px * c.size, y = c.y + py * c.size, rad = r * c.size;
            const g = ctx.createRadialGradient(x, y, 0, x, y, rad);
            g.addColorStop(0, `rgba(205, 225, 255, ${c.alpha})`);
            g.addColorStop(1, "rgba(205, 225, 255, 0)");
            ctx.fillStyle = g;
            ctx.beginPath();
            ctx.arc(x, y, rad, 0, Math.PI * 2);
            ctx.fill();
          }
        }
      }
      requestAnimationFrame(draw);
    }

    // Only draw while the hero is on screen
    new IntersectionObserver(entries => { visible = entries[0].isIntersecting; }).observe(sky);
    resize();
    window.addEventListener("resize", resize);
    requestAnimationFrame(draw);
  }
})();
