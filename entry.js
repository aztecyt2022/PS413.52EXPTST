(function(){
  "use strict";
  var started=false;
  var login=document.getElementById("loginPanel");
  var host=document.getElementById("hostPanel");
  var password=document.getElementById("password");
  var error=document.getElementById("loginError");
  function enterHost(){
    if(started||!window.HEN_AUTH.allowed())return;
    started=true;
    window.HEN_OVERLAY_TEST=/(?:[?&])overlay=1(?:&|$)/.test(location.search||"");
    document.getElementById("hostTitle").textContent=window.HEN_OVERLAY_TEST?"HEN + System Overlay test":"PS4 HEN 2.2.0";
    document.getElementById("overlayNote").hidden=!window.HEN_OVERLAY_TEST;
    document.getElementById("toggleAuto").hidden=window.HEN_OVERLAY_TEST;
    var link=document.getElementById("switchBuild");
    link.href=window.HEN_OVERLAY_TEST?"./":"?overlay=1";
    link.textContent=window.HEN_OVERLAY_TEST?"Use standard HEN":"Test System Overlay addition";
    password.value="";error.textContent="";login.hidden=true;host.hidden=false;
    window.startHENHost();
  }
  document.getElementById("loginForm").addEventListener("submit",function(event){
    event.preventDefault();
    var result=window.HEN_AUTH.enter(password.value);
    if(result==="ok")enterHost();
    else{
      error.textContent=result==="password"?"Incorrect password. The password is case-sensitive. Try again.":"Browser storage is unavailable. Enable it and try again.";
      password.value="";password.focus();
    }
  });
  document.getElementById("logout").addEventListener("click",function(){window.HEN_AUTH.logout();location.replace("./");});
  enterHost();
})();
