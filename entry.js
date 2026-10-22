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
    var build=window.HEN_BUILD;
    window.HEN_OVERLAY_TEST=build.manualOnly;
    document.getElementById("hostTitle").textContent=build.label+(build.manualOnly?" (manual)":"");
    document.getElementById("overlayNote").hidden=build.original;
    document.getElementById("toggleAuto").hidden=window.HEN_OVERLAY_TEST;
    var link=document.getElementById("switchBuild");
    link.href=build.original?"./":"?original=1";
    link.textContent=build.original?"Use PROHEN":"Use original HEN";
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
