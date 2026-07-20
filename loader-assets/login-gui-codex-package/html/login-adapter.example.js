(() => {
  "use strict";

  // Replace the fallback body with the app's REAL existing login logic.
  async function authenticateWithExistingLogic({ username, password, remember }) {
    if (window.auth?.login) return window.auth.login({ username, password, remember });

    // Browser preview only. Delete this demo block when integrating.
    await delay(350);
    if (username.toLowerCase() === "demo" && password === "demo") {
      return { ok: true, message: "Welcome back." };
    }
    throw new Error("Demo login is demo / demo. Connect the existing login function here.");
  }

  // This must call the REAL account-creation endpoint/service.
  async function registerWithExistingLogic({ fullName, username, email, password, acceptTerms }) {
    if (window.auth?.register) {
      return window.auth.register({ fullName, username, email, password, acceptTerms });
    }

    // Browser preview only. Any valid form values are accepted.
    await delay(450);
    return {
      ok: true,
      requiresVerification: true,
      fullName,
      username,
      email,
      message: "Account details accepted. Check your email."
    };
  }

  // This must validate the code through the REAL authentication service.
  async function verifyWithExistingLogic({ email, code }) {
    if (window.auth?.verifyEmail) return window.auth.verifyEmail({ email, code });

    // Browser preview only. Delete this demo block when integrating.
    await delay(400);
    if (code !== "123456") throw new Error("Demo verification code is 123456.");
    return { ok: true, message: "Email verified successfully." };
  }

  async function resendWithExistingLogic({ email }) {
    if (window.auth?.resendVerification) return window.auth.resendVerification({ email });

    // Browser preview only.
    await delay(300);
    return { ok: true, message: `A new demo code was sent to ${email}.` };
  }

  async function selectInstallationPath({ currentPath }) {
    if (window.installer?.selectPath) return window.installer.selectPath(currentPath);

    // Browser preview fallback. Electron uses a real native folder picker.
    const selected = window.prompt("Select installation folder", currentPath || "C:\\Program Files\\NEXUS");
    return selected ? { path: selected, canceled: false } : { path: "", canceled: true };
  }

  async function continueExistingInstallation({ installPath }) {
    if (window.installer?.load) return window.installer.load({ installPath });

    // Browser preview only.
    await delay(500);
    return { ok: true, message: `LOAD accepted: ${installPath}` };
  }

  function delay(milliseconds) {
    return new Promise(resolve => setTimeout(resolve, milliseconds));
  }

  window.LoginUI.setSubmitHandler(authenticateWithExistingLogic);
  window.LoginUI.setForgotPasswordHandler(username => {
    if (window.auth?.forgotPassword) return window.auth.forgotPassword(username);
    console.info("Connect this hook to the existing forgot-password flow.", username);
  });

  window.AccountUI.setRegisterHandler(registerWithExistingLogic);
  window.AccountUI.setVerifyHandler(verifyWithExistingLogic);
  window.AccountUI.setResendVerificationHandler(resendWithExistingLogic);
  window.AccountUI.setTermsHandler(() => console.info("Open the real Terms page or modal here."));
  window.AccountUI.setPrivacyHandler(() => console.info("Open the real Privacy Policy page or modal here."));

  window.InstallUI.setSelectPathHandler(selectInstallationPath);
  window.InstallUI.setLoadHandler(continueExistingInstallation);
})();
