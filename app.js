window.startHENHost=function () {
  "use strict";
  var build=window.HEN_BUILD;
  var loadButton=document.getElementById("loadExploit");
  var autoButton=document.getElementById("toggleAuto");
  var firmwareText=document.getElementById("firmware");
  var statusText=document.getElementById("status");
  var firmware=window.HEN_DEVICE.firmware;
  var supported=firmware==="13.52", timer=null, deadline=0, autoEnabled=false, busy=false;
  var preferenceKey="hen-autoload-disabled";
  var files=[
    ["loader/"+build.payload[0],build.payload[1],build.payload[2]],
    ["loader/patches/1352.bin",632,"adfb9771904f71cd1f5a82cbded8ac46c7ffeb3df974579535cf711904ac347e"]
  ];
  firmwareText.textContent="Detected: "+(firmware?"PS4 "+firmware:"not a PS4");
  function stopTimer(){if(timer!==null){clearInterval(timer);timer=null;}}
  function savePreference(disabled){try{localStorage.setItem(preferenceKey,disabled?"1":"0");}catch(_){}}
  function fileCheck(spec){return new Promise(function(resolve,reject){
    var xhr=new XMLHttpRequest();xhr.open("GET",spec[0],true);xhr.responseType="arraybuffer";xhr.timeout=20000;
    xhr.onload=function(){
      if(xhr.status===401){reject(Error("Login expired. Reload and enter the password again."));return;}
      if(xhr.status!==200||!xhr.response){reject(Error("Could not load the host files. Please try again."));return;}
      var bytes=new Uint8Array(xhr.response);
      if(bytes.length!==spec[1]||typeof window.HEN_SHA256!=="function"||window.HEN_SHA256(bytes)!==spec[2]){reject(Error("Host file verification failed. Reload the page before trying again."));return;}
      resolve(true);
    };
    xhr.onerror=xhr.ontimeout=function(){reject(Error("Could not load the host files. Check your connection."));};xhr.send();
  });}
  function launch(){
    if(!supported||busy)return;
    if(!window.HEN_AUTH.allowed()){
      stopTimer();autoEnabled=false;loadButton.disabled=true;autoButton.disabled=true;
      statusText.textContent="Login expired. Reload and enter the password again.";
      return;
    }
    stopTimer();autoEnabled=false;busy=true;
    loadButton.disabled=true;autoButton.disabled=true;
    autoButton.textContent="Enable auto loading";
    statusText.textContent="Checking exploit files…";
    Promise.all(files.map(fileCheck)).then(function(){
      statusText.textContent="Loading exploit…";
      window.location.assign("loader/jb.html"+build.query+(build.query?"&":"?")+"v=dexcore-ui-1-1");
    }).catch(function(error){
      busy=false;loadButton.disabled=!supported;autoButton.disabled=!supported;
      statusText.textContent=error.message+" Auto loading is stopped.";
    });
  }
  function tick(){
    if(!autoEnabled||!supported||busy)return;
    var remaining=Math.max(0,Math.ceil((deadline-Date.now())/1000));
    statusText.textContent="Auto loading in "+remaining+" second"+(remaining===1?".":"s.");
    if(remaining===0)launch();
  }
  function startAuto(){
    if(!supported||busy)return;
    stopTimer();autoEnabled=true;deadline=Date.now()+10000;
    autoButton.textContent="Disable auto loading";
    tick();timer=setInterval(tick,250);
  }
  loadButton.addEventListener("click",launch);
  autoButton.addEventListener("click",function(){
    if(!supported||busy)return;
    if(autoEnabled){stopTimer();autoEnabled=false;savePreference(true);autoButton.textContent="Enable auto loading";statusText.textContent="Auto loading disabled. Use Load exploit when ready.";}
    else{savePreference(false);startAuto();}
  });
  if(!supported){
    loadButton.disabled=true;autoButton.disabled=true;
    autoButton.textContent="Auto loading disabled";
    statusText.textContent="Disabled. Only a PS4 running firmware 13.52 is supported.";
    return;
  }
  loadButton.disabled=false;autoButton.disabled=false;
  var disabled=false;try{disabled=localStorage.getItem(preferenceKey)==="1";}catch(_){}
  if(disabled){autoButton.textContent="Enable auto loading";statusText.textContent="Auto loading disabled. Use Load exploit when ready.";}
  else startAuto();
};
