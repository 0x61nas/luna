// this is the dumbest implementation possable for the fmode by the Clancker
(function(){
    if (window.__fmode_installed) return;
    window.__fmode_installed = true;

    const chars = "asdfghjklqwertyuiopzxcvbnm";

    function gen2(){
        let s="";
        for(let i=0;i<2;i++) s+=chars[Math.floor(Math.random()*chars.length)];
        return s;
    }

    function visible(el){
        const r = el.getBoundingClientRect();
        return r.width>0 && r.height>0 &&
               r.bottom>0 && r.right>0 &&
               r.top < window.innerHeight &&
               r.left < window.innerWidth;
    }

    function getTargets(){
        return Array.from(document.querySelectorAll(
            "a,button,input,[onclick],[role=button]"
        )).filter(visible);
    }


    function cleanup(){
        window.__fmode_labels.forEach(l=>l.remove());
        window.__fmode_active=false;
        window.__fmode_buf="";
        document.removeEventListener("keydown", handler, true);
    }

    function start(){
        if (window.__fmode_active) return;
        window.__fmode_active = true;

        const els = getTargets();
        window.__fmode_map = Object.create(null);
        window.__fmode_labels = [];
        window.__fmode_buf = "";

        els.forEach(el=>{
            let key;
            do { key = gen2(); } while (window.__fmode_map[key]);

            const r = el.getBoundingClientRect();
            const d = document.createElement("div");
            d.textContent = key;
            d.style.position="fixed";
            d.style.left = r.left + "px";
            d.style.top  = r.top  + "px";
            d.style.background="yellow";
            d.style.color="black";
            d.style.fontSize="12px";
            d.style.padding="2px";
            d.style.zIndex=2147483647;

            document.body.appendChild(d);
            window.__fmode_map[key] = el;
            window.__fmode_labels.push(d);
        });


        function handler(e){
            if(!window.__fmode_active) return;

            if(e.key.length === 1){
                window.__fmode_buf += e.key.toLowerCase();
                if(window.__fmode_buf.length === 2){
                    const el = window.__fmode_map[window.__fmode_buf];
                    if(el) el.click();
                    cleanup();
                }
                e.preventDefault();
            }
        }

        document.addEventListener("keydown", handler, true);
    }

    window.__fmode_start = start;
    window.__fmode_cleanup = cleanup;
})();
