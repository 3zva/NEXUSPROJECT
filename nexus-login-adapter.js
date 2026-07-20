(() => {
  "use strict";

  const api = async (path, payload = {}) => {
    const response = await fetch(path, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(payload)
    });
    const data = await response.json();
    if (!data.ok) throw new Error(data.message || "Request failed.");
    return data;
  };

  window.LoginUI.setSubmitHandler(({ username, password, remember }) =>
    api("/api/login", { username, password, remember })
  );

  window.LoginUI.setForgotPasswordHandler(() => {
    throw new Error("Password recovery is not configured for this installer.");
  });

  window.AccountUI.setRegisterHandler(({ fullName, username, email, password, acceptTerms }) =>
    api("/api/register", { fullName, username, email, password, acceptTerms })
  );

  window.AccountUI.setVerifyHandler(({ email, code }) =>
    api("/api/verify", { email, code })
  );

  window.AccountUI.setResendVerificationHandler(({ email }) =>
    api("/api/resend", { email })
  );

  window.AccountUI.setTermsHandler(() => {
    console.info("Terms are not configured for this installer.");
  });

  window.AccountUI.setPrivacyHandler(() => {
    console.info("Privacy Policy is not configured for this installer.");
  });

  window.InstallUI.setSelectPathHandler(({ currentPath }) =>
    api("/api/select-path", { currentPath })
  );

  window.InstallUI.setLoadHandler(({ installPath }) =>
    api("/api/load", { installPath })
  );

  fetch("/api/session", { method: "POST", headers: { "Content-Type": "application/json" }, body: "{}" })
    .then(response => response.json())
    .then(data => {
      if (!data || !data.ok) return;
      const username = document.getElementById("username");
      const remember = document.getElementById("remember");
      if (username && data.email) username.value = data.email;
      if (remember) remember.checked = Boolean(data.remember);
      if (data.installPath && window.InstallUI?.setPath) window.InstallUI.setPath(data.installPath);
      if (data.email && data.remember && window.LoginUI?.showInstallPage) {
        window.LoginUI.showInstallPage({ username: data.email });
      }
    })
    .catch(() => {});
})();
