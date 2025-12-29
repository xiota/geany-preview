// Minimal DOM patcher: replaces only changed components
function parseHTML(html) {
  return new DOMParser().parseFromString(html, 'text/html');
}

function applyUserStyles(doc) {
  const head = document.head;

  // Remove all existing <link rel="stylesheet"> and <style> tags
  head.querySelectorAll('link[rel="stylesheet"], style').forEach(el => el.remove());

  // Add new <style> tags
  doc.querySelectorAll('style').forEach(s => {
    head.appendChild(s.cloneNode(true));
  });

  // Add new <link rel="stylesheet"> tags
  doc.querySelectorAll('link[rel="stylesheet"]').forEach(l => {
    head.appendChild(l.cloneNode(true));
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

  const oldNodes = Array.from(root.childNodes);
  const newNodes = Array.from(nextBody.childNodes);

  const len = Math.max(oldNodes.length, newNodes.length);

  for (let i = 0; i < len; i++) {
    const oldNode = oldNodes[i];
    const newNode = newNodes[i];

    if (!oldNode && newNode) {
      root.appendChild(newNode.cloneNode(true));
    } else if (oldNode && !newNode) {
      root.removeChild(oldNode);
    } else if (oldNode && newNode) {
      if (!nodesEqual(oldNode, newNode)) {
        root.replaceChild(newNode.cloneNode(true), oldNode);
      }
    }
  }
}

function nodesEqual(a, b) {
  if (a.nodeType !== b.nodeType) {
    return false;
  }

  if (a.nodeType === Node.TEXT_NODE) {
    return a.textContent === b.textContent;
  }

  if (a.tagName !== b.tagName) {
    return false;
  }

  const aAttrs = a.getAttributeNames();
  const bAttrs = b.getAttributeNames();
  if (aAttrs.length !== bAttrs.length) {
    return false;
  }

  for (const name of aAttrs) {
    if (a.getAttribute(name) !== b.getAttribute(name)) {
      return false;
    }
  }

  return a.textContent === b.textContent;
}
