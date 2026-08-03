(() => {
  "use strict";

  const state = {
    submitHandler: null,
    forgotPasswordHandler: null,
    createAccountHandler: null,
    registerHandler: null,
    verifyHandler: null,
    resendVerificationHandler: null,
    termsHandler: null,
    privacyHandler: null,
    selectPathHandler: null,
    loadHandler: null,
    loginBusy: false,
    registerBusy: false,
    verifyBusy: false,
    resendBusy: false,
    selectingPath: false,
    loadingInstaller: false,
    loadProgressTimer: null,
    loadProgressValue: 0,
    selectedPath: "",
    pendingAccount: { fullName: "", email: "", username: "" }
  };

  const elements = {};
  const byId = id => document.getElementById(id);

  function init() {
    elements.pages = Array.from(document.querySelectorAll(".page-shell"));

    elements.loginShell = byId("login-shell");
    elements.loginForm = byId("login-form");
    elements.username = byId("username");
    elements.password = byId("password");
    elements.remember = byId("remember");
    elements.loginStatus = byId("status");
    elements.loginSubmit = byId("submit-button");
    elements.togglePassword = byId("toggle-password");
    elements.passwordIcon = byId("password-icon");
    elements.forgotPassword = byId("forgot-password");
    elements.createAccount = byId("create-account");

    elements.registerShell = byId("register-shell");
    elements.registerForm = byId("register-form");
    elements.registerName = byId("register-name");
    elements.registerUsername = byId("register-username");
    elements.registerEmail = byId("register-email");
    elements.registerPassword = byId("register-password");
    elements.registerConfirm = byId("register-confirm");
    elements.registerTerms = byId("register-terms");
    elements.registerStatus = byId("register-status");
    elements.registerSubmit = byId("register-submit");
    elements.registerContinue = byId("register-continue");
    elements.registerBack = byId("register-back");
    elements.registerSignIn = Array.from(document.querySelectorAll(".register-sign-in"));
    elements.termsButton = byId("terms-button");
    elements.privacyButton = byId("privacy-button");
    elements.passwordToggles = Array.from(document.querySelectorAll(".password-toggle"));

    elements.verifyShell = byId("verify-shell");
    elements.verifyForm = byId("verify-form");
    elements.verificationEmail = byId("verification-email");
    elements.otpGroup = byId("otp-group");
    elements.otpInputs = Array.from(document.querySelectorAll(".otp-input"));
    elements.verificationCodeError = byId("verification-code-error");
    elements.verifyStatus = byId("verify-status");
    elements.verifySubmit = byId("verify-submit");
    elements.verifyBack = byId("verify-back");
    elements.resendCode = byId("resend-code");

    elements.successShell = byId("success-shell");
    elements.successMessage = byId("success-message");
    elements.successUsername = byId("success-username");
    elements.successEmail = byId("success-email");
    elements.continueToLogin = byId("continue-to-login");

    elements.installShell = byId("install-shell");
    elements.installForm = byId("install-form");
    elements.installPath = byId("install-path");
    elements.pathShell = byId("path-shell");
    elements.pathError = byId("path-error");
    elements.selectPath = byId("select-path-button");
    elements.load = byId("load-button");
    elements.installStatus = byId("install-status");
    elements.destinationPreview = byId("destination-preview");
    elements.authenticatedUser = byId("authenticated-user");
    elements.loadProgressShell = byId("load-progress-shell");
    elements.loadProgressStatus = byId("load-progress-status");
    elements.loadProgressStage = byId("load-progress-stage");
    elements.loadProgressPercent = byId("load-progress-percent");
    elements.loadProgressBar = byId("load-progress-bar");
    elements.loadProgressDetail = byId("load-progress-detail");

    elements.loginForm.addEventListener("submit", handleLoginSubmit);
    elements.togglePassword.addEventListener("click", () => toggleInputVisibility(elements.password, elements.togglePassword));
    elements.forgotPassword.addEventListener("click", handleForgotPassword);
    elements.createAccount.addEventListener("click", handleCreateAccountNavigation);
    [elements.username, elements.password].forEach(input => input.addEventListener("input", () => clearFieldError(input)));

    elements.registerForm.addEventListener("submit", handleRegisterSubmit);
    elements.registerBack.addEventListener("click", () => showLoginPage());
    elements.registerSignIn.forEach(button => button.addEventListener("click", () => showLoginPage()));
    elements.termsButton.addEventListener("click", () => invokePolicyHandler("terms"));
    elements.privacyButton.addEventListener("click", () => invokePolicyHandler("privacy"));
    elements.passwordToggles.forEach(button => {
      button.addEventListener("click", () => toggleInputVisibility(byId(button.dataset.input), button));
    });
    [elements.registerName, elements.registerUsername, elements.registerEmail, elements.registerPassword, elements.registerConfirm].forEach(input => {
      input.addEventListener("input", () => {
        clearFieldError(input);
        if (input === elements.registerPassword) updatePasswordRequirements();
        if (input === elements.registerPassword || input === elements.registerConfirm) clearFieldError(elements.registerConfirm);
      });
    });
    elements.registerTerms.addEventListener("change", () => {
      byId("register-terms-error").textContent = "";
      elements.registerTerms.removeAttribute("aria-invalid");
    });

    elements.verifyForm.addEventListener("submit", handleVerificationSubmit);
    elements.verifyBack.addEventListener("click", () => showRegisterPage({ restorePending: true }));
    elements.resendCode.addEventListener("click", handleResendCode);
    setupOtpInputs();

    elements.continueToLogin.addEventListener("click", () => {
      showLoginPage({ prefillUsername: state.pendingAccount.username || state.pendingAccount.email });
    });

    elements.installForm.addEventListener("submit", handleLoad);
    elements.selectPath.addEventListener("click", handleSelectPath);

    updatePasswordRequirements();
  }

  function showPage(pageName, focusTarget) {
    elements.pages.forEach(page => {
      page.hidden = page.dataset.page !== pageName;
      page.classList.remove("page-enter");
    });
    const page = elements.pages.find(item => item.dataset.page === pageName);
    if (!page) return;
    page.hidden = false;
    void page.offsetWidth;
    page.classList.add("page-enter");
    window.scrollTo({ top: 0, behavior: "auto" });
    if (focusTarget) window.requestAnimationFrame(() => focusTarget.focus());
  }

  function validateLogin() {
    let valid = true;
    clearFieldError(elements.username);
    clearFieldError(elements.password);
    if (!elements.username.value.trim()) {
      setFieldError(elements.username, "Enter your email or username.");
      valid = false;
    }
    if (!elements.password.value) {
      setFieldError(elements.password, "Enter your password.");
      valid = false;
    }
    return valid;
  }

  async function handleLoginSubmit(event) {
    event.preventDefault();
    if (state.loginBusy || !validateLogin()) return;
    if (typeof state.submitHandler !== "function") {
      setStatus(elements.loginStatus, "error", "No login handler is connected yet. Open login-adapter.example.js.");
      return;
    }

    const credentials = {
      username: elements.username.value.trim(),
      password: elements.password.value,
      remember: elements.remember.checked
    };

    setLoginBusy(true);
    setStatus(elements.loginStatus, "", "");
    try {
      const result = await state.submitHandler(credentials);
      assertSuccessfulResult(result, "Unable to sign in.");
      const message = objectMessage(result) || "Signed in successfully.";
      setStatus(elements.loginStatus, "success", message);
      document.dispatchEvent(new CustomEvent("login:success", { detail: { result, username: credentials.username } }));
      showInstallPage({ username: credentials.username });
    } catch (error) {
      const normalized = normalizeError(error, "Unable to sign in.");
      setStatus(elements.loginStatus, "error", normalized.message);
      document.dispatchEvent(new CustomEvent("login:error", { detail: { error: normalized, username: credentials.username } }));
    } finally {
      credentials.password = "";
      setLoginBusy(false);
    }
  }

  function handleForgotPassword() {
    if (typeof state.forgotPasswordHandler === "function") {
      state.forgotPasswordHandler(elements.username.value.trim());
    }
    document.dispatchEvent(new CustomEvent("login:forgot-password", { detail: { username: elements.username.value.trim() } }));
  }

  function handleCreateAccountNavigation() {
    if (typeof state.createAccountHandler === "function") state.createAccountHandler();
    document.dispatchEvent(new CustomEvent("login:create-account"));
    showRegisterPage();
  }

  function validateRegistration() {
    let valid = true;
    clearRegistrationErrors();

    const name = elements.registerName.value.trim();
    const username = elements.registerUsername.value.trim();
    const email = elements.registerEmail.value.trim();
    const password = elements.registerPassword.value;
    const confirmPassword = elements.registerConfirm.value;

    if (name.length < 2) {
      setFieldError(elements.registerName, "Enter your full name.");
      valid = false;
    }
    if (!/^[A-Za-z0-9_-]{3,24}$/.test(username)) {
      setFieldError(elements.registerUsername, "Use 3–24 letters, numbers, underscores, or hyphens.");
      valid = false;
    }
    if (!/^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(email)) {
      setFieldError(elements.registerEmail, "Enter a valid email address.");
      valid = false;
    }

    const requirements = getPasswordRequirements(password);
    if (!Object.values(requirements).every(Boolean)) {
      setFieldError(elements.registerPassword, "Meet all four password requirements.");
      valid = false;
    }
    if (!confirmPassword || password !== confirmPassword) {
      setFieldError(elements.registerConfirm, "Passwords do not match.");
      valid = false;
    }
    if (!elements.registerTerms.checked) {
      byId("register-terms-error").textContent = "Accept the Terms and Privacy Policy to continue.";
      elements.registerTerms.setAttribute("aria-invalid", "true");
      valid = false;
    }
    return valid;
  }

  async function handleRegisterSubmit(event) {
    event.preventDefault();
    if (state.registerBusy || !validateRegistration()) return;
    if (typeof state.registerHandler !== "function") {
      setStatus(elements.registerStatus, "error", "No account-registration handler is connected yet.");
      return;
    }

    const payload = {
      fullName: elements.registerName.value.trim(),
      username: elements.registerUsername.value.trim(),
      email: elements.registerEmail.value.trim(),
      password: elements.registerPassword.value,
      acceptTerms: elements.registerTerms.checked
    };

    setRegisterBusy(true);
    setStatus(elements.registerStatus, "", "");
    try {
      const result = await state.registerHandler(payload);
      assertSuccessfulResult(result, "Unable to create the account.");

      state.pendingAccount = {
        fullName: result && typeof result === "object" && typeof result.fullName === "string" ? result.fullName : payload.fullName,
        username: result && typeof result === "object" && typeof result.username === "string" ? result.username : payload.username,
        email: result && typeof result === "object" && typeof result.email === "string" ? result.email : payload.email
      };

      document.dispatchEvent(new CustomEvent("account:registered", { detail: { result, account: { ...state.pendingAccount } } }));

      const requiresVerification = !(result && typeof result === "object" && result.requiresVerification === false);
      if (requiresVerification) {
        showVerificationPage({ email: state.pendingAccount.email, username: state.pendingAccount.username });
      } else {
        showAccountCreatedPage({ message: objectMessage(result) || "Your account has been created." });
      }
    } catch (error) {
      const normalized = normalizeError(error, "Unable to create the account.");
      setStatus(elements.registerStatus, "error", normalized.message);
      document.dispatchEvent(new CustomEvent("account:registration-error", { detail: { error: normalized, email: payload.email } }));
    } finally {
      payload.password = "";
      setRegisterBusy(false);
    }
  }

  function setupOtpInputs() {
    elements.otpInputs.forEach((input, index) => {
      input.addEventListener("input", () => {
        input.value = input.value.replace(/\D/g, "").slice(-1);
        clearVerificationError();
        if (input.value && elements.otpInputs[index + 1]) elements.otpInputs[index + 1].focus();
      });
      input.addEventListener("keydown", event => {
        if (event.key === "Backspace" && !input.value && elements.otpInputs[index - 1]) elements.otpInputs[index - 1].focus();
        if (event.key === "ArrowLeft" && elements.otpInputs[index - 1]) elements.otpInputs[index - 1].focus();
        if (event.key === "ArrowRight" && elements.otpInputs[index + 1]) elements.otpInputs[index + 1].focus();
      });
    });

    elements.otpGroup.addEventListener("paste", event => {
      const digits = (event.clipboardData?.getData("text") || "").replace(/\D/g, "").slice(0, 6);
      if (!digits) return;
      event.preventDefault();
      elements.otpInputs.forEach((input, index) => { input.value = digits[index] || ""; });
      clearVerificationError();
      elements.otpInputs[Math.min(digits.length, 6) - 1]?.focus();
    });
  }

  function getVerificationCode() {
    return elements.otpInputs.map(input => input.value).join("");
  }

  function validateVerification() {
    const code = getVerificationCode();
    clearVerificationError();
    if (!/^\d{6}$/.test(code)) {
      elements.otpGroup.classList.add("has-error");
      elements.verificationCodeError.textContent = "Enter the complete six-digit code.";
      return false;
    }
    return true;
  }

  async function handleVerificationSubmit(event) {
    event.preventDefault();
    if (state.verifyBusy || !validateVerification()) return;
    if (typeof state.verifyHandler !== "function") {
      setStatus(elements.verifyStatus, "error", "No verification handler is connected yet.");
      return;
    }

    const payload = { email: state.pendingAccount.email, code: getVerificationCode() };
    setVerifyBusy(true);
    setStatus(elements.verifyStatus, "", "");
    try {
      const result = await state.verifyHandler(payload);
      assertSuccessfulResult(result, "Unable to verify the account.");
      document.dispatchEvent(new CustomEvent("account:verified", { detail: { result, account: { ...state.pendingAccount } } }));
      showAccountCreatedPage({ message: objectMessage(result) || "Your email has been verified and your account is ready." });
    } catch (error) {
      const normalized = normalizeError(error, "Unable to verify the account.");
      setStatus(elements.verifyStatus, "error", normalized.message);
      document.dispatchEvent(new CustomEvent("account:verification-error", { detail: { error: normalized, email: payload.email } }));
    } finally {
      payload.code = "";
      setVerifyBusy(false);
    }
  }

  async function handleResendCode() {
    if (state.resendBusy || state.verifyBusy) return;
    if (typeof state.resendVerificationHandler !== "function") {
      setStatus(elements.verifyStatus, "error", "No resend-code handler is connected yet.");
      return;
    }

    setResendBusy(true);
    setStatus(elements.verifyStatus, "", "");
    try {
      const result = await state.resendVerificationHandler({ email: state.pendingAccount.email });
      assertSuccessfulResult(result, "Unable to resend the code.");
      setStatus(elements.verifyStatus, "success", objectMessage(result) || "A new verification code was sent.");
      document.dispatchEvent(new CustomEvent("account:verification-resent", { detail: { result, email: state.pendingAccount.email } }));
    } catch (error) {
      setStatus(elements.verifyStatus, "error", normalizeError(error, "Unable to resend the code.").message);
    } finally {
      setResendBusy(false);
    }
  }

  async function handleSelectPath() {
    if (state.selectingPath || state.loadingInstaller) return;
    if (typeof state.selectPathHandler !== "function") {
      setStatus(elements.installStatus, "error", "No path-selection handler is connected yet.");
      return;
    }

    clearPathError();
    setStatus(elements.installStatus, "", "");
    setPathBusy(true);
    try {
      const result = await state.selectPathHandler({ currentPath: state.selectedPath });
      const normalized = normalizePathResult(result);
      if (normalized.canceled) return;
      if (!normalized.path) throw new Error("No folder was selected.");
      setSelectedPath(normalized.path);
      setStatus(elements.installStatus, "success", "Installation path selected.");
      document.dispatchEvent(new CustomEvent("installer:path-selected", { detail: { installPath: normalized.path } }));
    } catch (error) {
      setStatus(elements.installStatus, "error", normalizeError(error, "Unable to select a folder.").message);
    } finally {
      setPathBusy(false);
    }
  }

  async function handleLoad(event) {
    event.preventDefault();
    if (state.loadingInstaller || state.selectingPath) return;
    if (!state.selectedPath) {
      setPathError("Select an installation folder first.");
      return;
    }
    if (typeof state.loadHandler !== "function") {
      setStatus(elements.installStatus, "error", "No LOAD handler is connected yet.");
      return;
    }

    showLoadProgress("STARTING", "Loading NEXUS client...", 8, "The application will open after readiness is complete.");
    setStatus(elements.installStatus, "", "");
    const payload = { installPath: state.selectedPath };
    try {
      startLoadProgressLoop();
      const result = await state.loadHandler(payload);
      assertSuccessfulResult(result, "Unable to continue installation.");
      stopLoadProgressLoop();
      updateLoadProgress("FINALIZING", "NEXUS is almost ready...", Math.max(state.loadProgressValue, 96), objectMessage(result) || "Finishing the client handoff.");
      document.dispatchEvent(new CustomEvent("installer:load", { detail: { result, installPath: state.selectedPath } }));
      window.setTimeout(() => updateLoadProgress("READY", "NEXUS is ready.", 100, "Opening the application."), 180);
    } catch (error) {
      const normalized = normalizeError(error, "Unable to continue installation.");
      stopLoadProgressLoop();
      updateLoadProgress("LOAD FAILED", normalized.message, state.loadProgressValue || 100, "Return to the load page and try again.");
      window.setTimeout(() => showInstallPage({ username: state.pendingAccount.username || "" }), 1200);
      setStatus(elements.installStatus, "error", normalized.message);
      document.dispatchEvent(new CustomEvent("installer:error", { detail: { error: normalized, installPath: state.selectedPath } }));
      setLoadBusy(false);
    }
  }

  function showLoginPage({ prefillUsername = "" } = {}) {
    if (prefillUsername) elements.username.value = prefillUsername;
    setStatus(elements.loginStatus, "", "");
    showPage("login", elements.username);
  }

  function showRegisterPage({ restorePending = false, email = "" } = {}) {
    if (restorePending) {
      elements.registerName.value = state.pendingAccount.fullName;
      elements.registerUsername.value = state.pendingAccount.username;
      elements.registerEmail.value = state.pendingAccount.email;
    } else if (email) {
      elements.registerEmail.value = email;
    }
    setStatus(elements.registerStatus, "", "");
    updatePasswordRequirements();
    showPage("register", elements.registerName);
  }

  function showVerificationPage({ email = "", username = "" } = {}) {
    if (email) state.pendingAccount.email = email;
    if (username) state.pendingAccount.username = username;
    elements.verificationEmail.textContent = state.pendingAccount.email || "your email";
    clearOtp();
    setStatus(elements.verifyStatus, "", "");
    showPage("verify", elements.otpInputs[0]);
  }

  function showAccountCreatedPage({ message = "" } = {}) {
    elements.successUsername.textContent = state.pendingAccount.username || "New user";
    elements.successEmail.textContent = state.pendingAccount.email || "Verified";
    elements.successMessage.textContent = message || "Your email has been verified and your account is ready.";
    showPage("success", elements.continueToLogin);
  }

  function showInstallPage({ username = "" } = {}) {
    elements.authenticatedUser.textContent = username ? `Authenticated as ${username}` : "Authentication complete";
    showPage("install", elements.selectPath);
  }

  function getPasswordRequirements(password) {
    return {
      length: password.length >= 8,
      upper: /[A-Z]/.test(password),
      lower: /[a-z]/.test(password),
      number: /\d/.test(password)
    };
  }

  function updatePasswordRequirements() {
    const requirements = getPasswordRequirements(elements.registerPassword.value);
    Object.entries(requirements).forEach(([key, met]) => byId(`rule-${key}`).classList.toggle("met", met));
  }

  function toggleInputVisibility(input, button) {
    const show = input.type === "password";
    input.type = show ? "text" : "password";
    button.setAttribute("aria-label", show ? "Hide password" : "Show password");
    button.setAttribute("aria-pressed", String(show));
    const icon = button.querySelector("img");
    if (icon) icon.src = show ? "../assets/eye-off.svg" : "../assets/eye.svg";
  }

  function setFieldError(input, message) {
    input.closest(".input-shell")?.classList.add("has-error");
    const error = byId(`${input.id}-error`);
    if (error) error.textContent = message;
    input.setAttribute("aria-invalid", "true");
  }

  function clearFieldError(input) {
    if (!input) return;
    input.closest(".input-shell")?.classList.remove("has-error");
    const error = byId(`${input.id}-error`);
    if (error) error.textContent = "";
    input.removeAttribute("aria-invalid");
  }

  function clearRegistrationErrors() {
    [elements.registerName, elements.registerUsername, elements.registerEmail, elements.registerPassword, elements.registerConfirm].forEach(clearFieldError);
    byId("register-terms-error").textContent = "";
    elements.registerTerms.removeAttribute("aria-invalid");
  }

  function clearVerificationError() {
    elements.otpGroup.classList.remove("has-error");
    elements.verificationCodeError.textContent = "";
  }

  function clearOtp() {
    elements.otpInputs.forEach(input => { input.value = ""; });
    clearVerificationError();
  }

  function setPathError(message) {
    elements.pathShell.classList.add("has-error");
    elements.pathError.textContent = message;
    elements.installPath.setAttribute("aria-invalid", "true");
  }

  function clearPathError() {
    elements.pathShell.classList.remove("has-error");
    elements.pathError.textContent = "";
    elements.installPath.removeAttribute("aria-invalid");
  }

  function setSelectedPath(path) {
    state.selectedPath = String(path || "").trim();
    elements.installPath.value = state.selectedPath;
    elements.pathShell.classList.toggle("has-selection", Boolean(state.selectedPath));
    elements.destinationPreview.textContent = state.selectedPath || "Waiting for a folder";
    elements.load.disabled = !state.selectedPath || state.loadingInstaller || state.selectingPath;
    clearPathError();
  }

  function setStatus(element, type, message) {
    element.className = `status${element === elements.installStatus ? " install-status" : ""}${type ? ` ${type}` : ""}`;
    element.textContent = message;
  }

  function setLoginBusy(busy) {
    state.loginBusy = busy;
    [elements.username, elements.password, elements.remember, elements.loginSubmit].forEach(element => { element.disabled = busy; });
    elements.loginSubmit.classList.toggle("loading", busy);
    elements.loginSubmit.querySelector(".button-label").textContent = busy ? "Signing in" : "Sign in";
  }

  function setRegisterBusy(busy) {
    state.registerBusy = busy;
    elements.registerForm.querySelectorAll("input, button").forEach(element => { element.disabled = busy; });
    elements.registerContinue.disabled = busy;
    elements.registerSubmit.classList.toggle("loading", busy);
    elements.registerContinue.classList.toggle("loading", busy);
    elements.registerSubmit.querySelector(".button-label").textContent = busy ? "Creating account" : "Create account";
    elements.registerContinue.querySelector(".button-label").textContent = busy ? "Creating account" : "Continue";
  }

  function setVerifyBusy(busy) {
    state.verifyBusy = busy;
    elements.otpInputs.forEach(input => { input.disabled = busy; });
    elements.verifySubmit.disabled = busy;
    elements.verifyBack.disabled = busy;
    elements.resendCode.disabled = busy || state.resendBusy;
    elements.verifySubmit.classList.toggle("loading", busy);
    elements.verifySubmit.querySelector(".button-label").textContent = busy ? "Verifying" : "Verify account";
  }

  function setResendBusy(busy) {
    state.resendBusy = busy;
    elements.resendCode.disabled = busy || state.verifyBusy;
    elements.resendCode.textContent = busy ? "Sending…" : "Resend code";
  }

  function setPathBusy(busy) {
    state.selectingPath = busy;
    elements.selectPath.disabled = busy;
    elements.load.disabled = busy || !state.selectedPath || state.loadingInstaller;
    elements.selectPath.textContent = busy ? "Selecting…" : "Browse";
  }

  function setLoadBusy(busy) {
    state.loadingInstaller = busy;
    elements.selectPath.disabled = busy;
    elements.load.disabled = busy || !state.selectedPath;
    elements.load.classList.remove("loading");
    elements.load.querySelector(".button-label").textContent = "LOAD";
  }

  function showLoadProgress(stage, status, percent, detail) {
    setLoadBusy(true);
    state.loadProgressValue = 0;
    stopLoadProgressLoop();
    updateLoadProgress(stage, status, percent, detail);
    showPage("load-progress");
  }

  function updateLoadProgress(stage, status, percent, detail) {
    const bounded = Math.max(0, Math.min(100, Number(percent) || 0));
    elements.loadProgressStage.textContent = stage || "LOADING";
    elements.loadProgressStatus.textContent = status || "Loading NEXUS client...";
    elements.loadProgressPercent.textContent = `${Math.round(bounded)}%`;
    elements.loadProgressBar.style.width = `${bounded}%`;
    elements.loadProgressDetail.textContent = detail || "";
    state.loadProgressValue = bounded;
  }

  function startLoadProgressLoop() {
    stopLoadProgressLoop();
    const stages = [
      { at: 12, stage: "PREPARING", status: "Preparing files and runtime services...", detail: "Applying the selected installation path." },
      { at: 34, stage: "CONFIGURING", status: "Configuring the client environment...", detail: "Copying required runtime files and saved settings." },
      { at: 58, stage: "STARTING", status: "Starting NEXUS services...", detail: "Launching the client components." },
      { at: 78, stage: "CONNECTING", status: "Connecting NEXUS to the launch flow...", detail: "Checking that the handoff is ready." },
      { at: 90, stage: "FINALIZING", status: "Finalizing startup...", detail: "Waiting for the last startup tasks." }
    ];

    state.loadProgressTimer = window.setInterval(() => {
      const current = state.loadProgressValue;
      if (current >= 94) return;
      const increment = current < 35 ? 2.4 : current < 70 ? 1.35 : 0.45;
      const next = Math.min(94, current + increment);
      const stage = stages.reduce((selected, item) => next >= item.at ? item : selected, stages[0]);
      updateLoadProgress(stage.stage, stage.status, next, stage.detail);
    }, 180);
  }

  function stopLoadProgressLoop() {
    if (state.loadProgressTimer) {
      window.clearInterval(state.loadProgressTimer);
      state.loadProgressTimer = null;
    }
  }

  function normalizePathResult(result) {
    if (typeof result === "string") return { path: result.trim(), canceled: false };
    if (!result || typeof result !== "object") return { path: "", canceled: true };
    return { path: typeof result.path === "string" ? result.path.trim() : "", canceled: Boolean(result.canceled) };
  }

  function normalizeError(error, fallback) {
    return error instanceof Error ? error : new Error(fallback);
  }

  function objectMessage(result) {
    return result && typeof result === "object" && typeof result.message === "string" ? result.message : "";
  }

  function assertSuccessfulResult(result, fallback) {
    if (result === false || (result && typeof result === "object" && result.ok === false)) {
      throw new Error(objectMessage(result) || fallback);
    }
  }

  function invokePolicyHandler(kind) {
    const handler = kind === "terms" ? state.termsHandler : state.privacyHandler;
    if (typeof handler === "function") handler();
    document.dispatchEvent(new CustomEvent(kind === "terms" ? "account:terms" : "account:privacy"));
  }

  window.LoginUI = Object.freeze({
    setSubmitHandler(handler) { requireFunction(handler, "Login submit handler"); state.submitHandler = handler; },
    setForgotPasswordHandler(handler) { requireFunction(handler, "Forgot-password handler"); state.forgotPasswordHandler = handler; },
    setCreateAccountHandler(handler) { requireFunction(handler, "Create-account navigation handler"); state.createAccountHandler = handler; },
    setStatus(type, message) { setStatus(elements.loginStatus, type, message); },
    setBusy: setLoginBusy,
    showInstallPage,
    showLoginPage
  });

  window.AccountUI = Object.freeze({
    setRegisterHandler(handler) { requireFunction(handler, "Account registration handler"); state.registerHandler = handler; },
    setVerifyHandler(handler) { requireFunction(handler, "Account verification handler"); state.verifyHandler = handler; },
    setResendVerificationHandler(handler) { requireFunction(handler, "Verification resend handler"); state.resendVerificationHandler = handler; },
    setTermsHandler(handler) { requireFunction(handler, "Terms handler"); state.termsHandler = handler; },
    setPrivacyHandler(handler) { requireFunction(handler, "Privacy handler"); state.privacyHandler = handler; },
    showRegisterPage,
    showVerificationPage,
    showSuccessPage: showAccountCreatedPage,
    showLoginPage,
    getPendingAccount() { return { ...state.pendingAccount }; }
  });

  window.InstallUI = Object.freeze({
    setSelectPathHandler(handler) { requireFunction(handler, "Path-selection handler"); state.selectPathHandler = handler; },
    setLoadHandler(handler) { requireFunction(handler, "LOAD handler"); state.loadHandler = handler; },
    setPath: setSelectedPath,
    setStatus(type, message) { setStatus(elements.installStatus, type, message); },
    setBusy: setLoadBusy,
    show: showInstallPage,
    showLoginPage,
    getPath() { return state.selectedPath; }
  });

  function requireFunction(handler, label) {
    if (typeof handler !== "function") throw new TypeError(`${label} must be a function.`);
  }

  if (document.readyState === "loading") document.addEventListener("DOMContentLoaded", init, { once: true });
  else init();
})();
