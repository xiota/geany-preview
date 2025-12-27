// Minimal DOM patcher: replaces only changed children of #root
function parseBody(html) {
  const doc = new DOMParser().parseFromString(html, 'text/html');
  return doc.body || document.createElement('body');
}

function applyPatch(newHtml, root_id) {
  root = document.getElementById(root_id);
  if (!root) return;

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
