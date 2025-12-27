// Minimal DOM patcher: replaces only changed components
function parseHTML(html) {
  return new DOMParser().parseFromString(html, 'text/html');
}

function applyUserStyles(doc) {
  doc.querySelectorAll('style').forEach(s => {
    document.head.appendChild(s.cloneNode(true));
  });

  doc.querySelectorAll('link[rel="stylesheet"]').forEach(l => {
    document.head.appendChild(l.cloneNode(true));
  });
}

function applyUserMeta(doc) {
  const viewport = doc.querySelector('meta[name="viewport"]');
  if (viewport) {
    document.head.appendChild(viewport.cloneNode(true));
  }
}

function applyUserTitle(doc) {
  const title = doc.querySelector('title');
  if (title) {
    document.title = title.textContent;
  }
}

function parseBody(html) {
  const doc = parseHTML(html);
  applyUserStyles(doc);
  applyUserMeta(doc);
  applyUserTitle(doc);
  return doc.body || document.createElement('body');
}

function applyPatch(newHtml, root_id) {
  const root = document.getElementById(root_id);
  if (!root) {
    return;
  }

  const nextBody = parseBody(newHtml);
  const oldChildren = Array.from(root.children);
  const newChildren = Array.from(nextBody.children);
  const len = Math.max(oldChildren.length, newChildren.length);

  for (let i = 0; i < len; i++) {
    const oldNode = oldChildren[i];
    const newNode = newChildren[i];

    if (!oldNode && newNode) {
      root.appendChild(newNode);
    } else if (oldNode && !newNode) {
      root.removeChild(oldNode);
    } else if (oldNode && newNode) {
      if (oldNode.outerHTML !== newNode.outerHTML) {
        root.replaceChild(newNode, oldNode);
      }
    }
  }
}
