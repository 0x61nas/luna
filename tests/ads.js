const DOMAIN_CATALOG = [
  {
    "id": "browser-baits",
    "name": "Browser bait checks",
    "description": "Local script-name and cosmetic-filter probes that reveal browser-extension filtering even when DNS is not used.",
    "tests": []
  },
  {
    "id": "core-ad-networks",
    "name": "Core ad networks",
    "description": "High-signal advertising delivery, ad-serving and programmatic exchange endpoints.",
    "tests": [
      {
        "service": "Google Ads / DoubleClick",
        "domains": [
          "pagead2.googlesyndication.com",
          "googleads.g.doubleclick.net",
          "securepubads.g.doubleclick.net",
          "ad.doubleclick.net",
          "stats.g.doubleclick.net",
          "tpc.googlesyndication.com",
          "partner.googleadservices.com",
          "pagead2.googleadservices.com",
          "adservice.google.com",
          "adservice.google.de",
          "afs.googlesyndication.com"
        ]
      },
      {
        "service": "Amazon Ads",
        "domains": [
          "aax.amazon-adsystem.com",
          "c.amazon-adsystem.com",
          "s.amazon-adsystem.com",
          "mads.amazon-adsystem.com",
          "fls-na.amazon-adsystem.com",
          "adtago.s3.amazonaws.com",
          "analyticsengine.s3.amazonaws.com",
          "analytics.s3.amazonaws.com",
          "advice-ads.s3.amazonaws.com"
        ]
      },
      {
        "service": "Microsoft Advertising",
        "domains": [
          "bat.bing.com",
          "ads.microsoft.com",
          "analytics.microsoft.com",
          "c.bing.com",
          "cdn.ads.microsoft.com",
          "bingads.microsoft.com"
        ]
      },
      {
        "service": "Yahoo / Verizon / AOL",
        "domains": [
          "ads.yahoo.com",
          "analytics.yahoo.com",
          "adtech.yahooinc.com",
          "udc.yahoo.com",
          "udcm.yahoo.com",
          "analytics.query.yahoo.com",
          "partnerads.ysm.yahoo.com",
          "gemini.yahoo.com",
          "log.fc.yahoo.com",
          "adserver-us.adtech.advertising.com"
        ]
      },
      {
        "service": "Criteo",
        "domains": [
          "static.criteo.net",
          "gum.criteo.com",
          "bidder.criteo.com",
          "sslwidget.criteo.com",
          "cas.criteo.com",
          "dis.criteo.com"
        ]
      },
      {
        "service": "Taboola",
        "domains": [
          "trc.taboola.com",
          "cdn.taboola.com",
          "images.taboola.com",
          "api.taboola.com",
          "vidstat.taboola.com"
        ]
      },
      {
        "service": "Outbrain",
        "domains": [
          "widgets.outbrain.com",
          "odb.outbrain.com",
          "amplifypixel.outbrain.com",
          "traffic.outbrain.com",
          "log.outbrain.com"
        ]
      },
      {
        "service": "Media.net",
        "domains": [
          "static.media.net",
          "contextual.media.net",
          "adservetx.media.net",
          "media.net"
        ]
      }
    ]
  },
  {
    "id": "programmatic-rtb",
    "name": "Programmatic / RTB",
    "description": "Demand-side, supply-side, header-bidding and real-time-bidding infrastructure.",
    "tests": [
      {
        "service": "PubMatic",
        "domains": [
          "ads.pubmatic.com",
          "hbopenbid.pubmatic.com",
          "image2.pubmatic.com",
          "gads.pubmatic.com",
          "ow.pubmatic.com"
        ]
      },
      {
        "service": "Magnite / Rubicon",
        "domains": [
          "fastlane.rubiconproject.com",
          "pixel.rubiconproject.com",
          "ads.rubiconproject.com",
          "eus.rubiconproject.com",
          "s.update.rubiconproject.com",
          "prebid-server.rubiconproject.com"
        ]
      },
      {
        "service": "OpenX",
        "domains": [
          "us-u.openx.net",
          "u.openx.net",
          "rtb.openx.net",
          "delivery-us.openx.net",
          "jp-u.openx.net"
        ]
      },
      {
        "service": "Xandr / AppNexus",
        "domains": [
          "ib.adnxs.com",
          "secure.adnxs.com",
          "acdn.adnxs.com",
          "sin3-ib.adnxs.com",
          "nym1-ib.adnxs.com",
          "adnxs.com"
        ]
      },
      {
        "service": "The Trade Desk",
        "domains": [
          "match.adsrvr.org",
          "insight.adsrvr.org",
          "js.adsrvr.org",
          "bid.adsrvr.org"
        ]
      },
      {
        "service": "Index Exchange / Casale",
        "domains": [
          "js-sec.indexww.com",
          "as-sec.casalemedia.com",
          "ssum-sec.casalemedia.com",
          "htlb.casalemedia.com",
          "r.casalemedia.com"
        ]
      },
      {
        "service": "Sovrn / Lijit",
        "domains": [
          "ap.lijit.com",
          "ce.lijit.com",
          "vap.lijit.com",
          "gslbeacon.lijit.com",
          "ads.sovrn.com"
        ]
      },
      {
        "service": "Adform",
        "domains": [
          "track.adform.net",
          "adx.adform.net",
          "s1.adform.net",
          "server.adform.net"
        ]
      },
      {
        "service": "Bidswitch / Sharethrough",
        "domains": [
          "x.bidswitch.net",
          "simage2.pubmatic.com",
          "native.sharethrough.com",
          "match.sharethrough.com"
        ]
      }
    ]
  },
  {
    "id": "analytics-telemetry",
    "name": "Analytics / telemetry",
    "description": "Web analytics, event collection, tag managers, heatmaps and product analytics.",
    "tests": [
      {
        "service": "Google Analytics / Tag Manager",
        "domains": [
          "www.google-analytics.com",
          "ssl.google-analytics.com",
          "google-analytics.com",
          "analytics.google.com",
          "region1.google-analytics.com",
          "www.googletagmanager.com",
          "googletagmanager.com"
        ]
      },
      {
        "service": "Microsoft Clarity",
        "domains": [
          "www.clarity.ms",
          "c.clarity.ms",
          "scripts.clarity.ms",
          "a.clarity.ms"
        ]
      },
      {
        "service": "Adobe Experience Cloud",
        "domains": [
          "assets.adobedtm.com",
          "dpm.demdex.net",
          "fast.demdex.net",
          "cm.everesttech.net",
          "adobedc.demdex.net",
          "sstats.adobe.com"
        ]
      },
      {
        "service": "Hotjar",
        "domains": [
          "script.hotjar.com",
          "static.hotjar.com",
          "identify.hotjar.com",
          "events.hotjar.io",
          "insights.hotjar.com",
          "adm.hotjar.com",
          "surveys.hotjar.com"
        ]
      },
      {
        "service": "Segment",
        "domains": [
          "cdn.segment.com",
          "api.segment.io",
          "events.segmentapis.com"
        ]
      },
      {
        "service": "Mixpanel",
        "domains": [
          "cdn.mxpnl.com",
          "api-js.mixpanel.com",
          "api.mixpanel.com",
          "decide.mixpanel.com"
        ]
      },
      {
        "service": "Amplitude",
        "domains": [
          "cdn.amplitude.com",
          "api2.amplitude.com",
          "regionconfig.amplitude.com",
          "api.eu.amplitude.com"
        ]
      },
      {
        "service": "FullStory",
        "domains": [
          "fullstory.com",
          "edge.fullstory.com",
          "rs.fullstory.com",
          "o-collector.fullstory.com"
        ]
      },
      {
        "service": "Mouseflow",
        "domains": [
          "mouseflow.com",
          "cdn.mouseflow.com",
          "o2.mouseflow.com",
          "gtm.mouseflow.com",
          "api.mouseflow.com",
          "tools.mouseflow.com"
        ]
      },
      {
        "service": "Lucky Orange",
        "domains": [
          "luckyorange.com",
          "api.luckyorange.com",
          "realtime.luckyorange.com",
          "cdn.luckyorange.com",
          "w1.luckyorange.com",
          "upload.luckyorange.net",
          "cs.luckyorange.net",
          "settings.luckyorange.net"
        ]
      },
      {
        "service": "Crazy Egg / Heap",
        "domains": [
          "script.crazyegg.com",
          "tracking.crazyegg.com",
          "cdn.heapanalytics.com",
          "heapanalytics.com",
          "c.heapanalytics.com"
        ]
      },
      {
        "service": "Plausible / Matomo",
        "domains": [
          "plausible.io",
          "cdn.matomo.cloud",
          "matomo.cloud",
          "stats.wp.com"
        ]
      }
    ]
  },
  {
    "id": "social-pixels",
    "name": "Social pixels",
    "description": "Social ad pixels, conversion APIs and behavioral event collectors.",
    "tests": [
      {
        "service": "Meta / Facebook",
        "domains": [
          "connect.facebook.net",
          "pixel.facebook.com",
          "an.facebook.com",
          "www.facebook.com",
          "graph.facebook.com"
        ]
      },
      {
        "service": "TikTok",
        "domains": [
          "analytics.tiktok.com",
          "ads.tiktok.com",
          "ads-api.tiktok.com",
          "analytics-sg.tiktok.com",
          "ads-sg.tiktok.com",
          "business-api.tiktok.com",
          "log.byteoversea.com",
          "mon.tiktokv.com"
        ]
      },
      {
        "service": "X / Twitter",
        "domains": [
          "static.ads-twitter.com",
          "analytics.twitter.com",
          "ads-api.twitter.com",
          "ads-twitter.com"
        ]
      },
      {
        "service": "LinkedIn",
        "domains": [
          "snap.licdn.com",
          "ads.linkedin.com",
          "px.ads.linkedin.com",
          "analytics.pointdrive.linkedin.com"
        ]
      },
      {
        "service": "Pinterest",
        "domains": [
          "ads.pinterest.com",
          "log.pinterest.com",
          "analytics.pinterest.com",
          "trk.pinterest.com"
        ]
      },
      {
        "service": "Reddit",
        "domains": [
          "events.reddit.com",
          "events.redditmedia.com",
          "alb.reddit.com"
        ]
      },
      {
        "service": "Snapchat",
        "domains": [
          "tr.snapchat.com",
          "ads.snapchat.com",
          "sc-static.net"
        ]
      },
      {
        "service": "Quora / Nextdoor",
        "domains": [
          "q.quora.com",
          "ads-api.nextdoor.com",
          "pixel.nextdoor.com"
        ]
      }
    ]
  },
  {
    "id": "mobile-app-ads",
    "name": "Mobile app ads / attribution",
    "description": "Mobile SDK ad delivery, attribution and in-app event collection endpoints.",
    "tests": [
      {
        "service": "Adjust",
        "domains": [
          "app.adjust.com",
          "app.adjust.net",
          "view.adjust.com",
          "app.adjust.world",
          "gdpr.adjust.com"
        ]
      },
      {
        "service": "AppsFlyer",
        "domains": [
          "app.appsflyer.com",
          "t.appsflyer.com",
          "events.appsflyer.com",
          "cdn-settings.appsflyersdk.com",
          "conversions.appsflyer.com"
        ]
      },
      {
        "service": "Branch",
        "domains": [
          "api2.branch.io",
          "cdn.branch.io",
          "sdk.branch.io",
          "v1.branch.io"
        ]
      },
      {
        "service": "Kochava",
        "domains": [
          "control.kochava.com",
          "kvinit-prod.api.kochava.com",
          "tracker.kochava.com",
          "imp.control.kochava.com"
        ]
      },
      {
        "service": "Unity Ads",
        "domains": [
          "auction.unityads.unity3d.com",
          "webview.unityads.unity3d.com",
          "config.unityads.unity3d.com",
          "adserver.unityads.unity3d.com"
        ]
      },
      {
        "service": "AppLovin",
        "domains": [
          "d.applovin.com",
          "a.applovin.com",
          "ms.applovin.com",
          "rt.applovin.com",
          "res1.applovin.com"
        ]
      },
      {
        "service": "Vungle / Liftoff",
        "domains": [
          "ads.api.vungle.com",
          "api.vungle.com",
          "config.ads.vungle.com",
          "events.ads.vungle.com"
        ]
      },
      {
        "service": "ironSource / Fyber / Inneractive",
        "domains": [
          "init.supersonicads.com",
          "outcome-ssp.supersonicads.com",
          "sdk-events.inner-active.mobi",
          "cdn2.inner-active.mobi",
          "ads.nexage.com"
        ]
      },
      {
        "service": "Chartboost",
        "domains": [
          "live.chartboost.com",
          "da.chartboost.com",
          "api.chartboost.com"
        ]
      },
      {
        "service": "AdColony",
        "domains": [
          "ads30.adcolony.com",
          "adc3-launch.adcolony.com",
          "events3alt.adcolony.com",
          "wd.adcolony.com"
        ]
      },
      {
        "service": "Mintegral / Rayjump",
        "domains": [
          "sdk-api.mintegral.com",
          "net.rayjump.com",
          "setting.rayjump.com",
          "detect.rayjump.com"
        ]
      },
      {
        "service": "InMobi",
        "domains": [
          "sdkm.w.inmobi.com",
          "telemetry.sdk.inmobi.com",
          "i.l.inmobicdn.net",
          "config.inmobi.com"
        ]
      }
    ]
  },
  {
    "id": "error-session-replay",
    "name": "Error, RUM and session replay",
    "description": "Bug/error tracking, crash analytics, replay and real-user-monitoring collectors.",
    "tests": [
      {
        "service": "Sentry",
        "domains": [
          "browser.sentry-cdn.com",
          "app.getsentry.com",
          "o0.ingest.sentry.io",
          "sentry.io"
        ]
      },
      {
        "service": "Bugsnag",
        "domains": [
          "notify.bugsnag.com",
          "sessions.bugsnag.com",
          "api.bugsnag.com",
          "app.bugsnag.com"
        ]
      },
      {
        "service": "Datadog RUM",
        "domains": [
          "browser-intake-datadoghq.com",
          "browser-intake-datadoghq.eu",
          "rum.browser-intake-datadoghq.com"
        ]
      },
      {
        "service": "New Relic",
        "domains": [
          "bam.nr-data.net",
          "js-agent.newrelic.com",
          "bam-cell.nr-data.net"
        ]
      },
      {
        "service": "Rollbar / Raygun",
        "domains": [
          "api.rollbar.com",
          "browser-intake-us5-datadoghq.com",
          "api.raygun.io"
        ]
      },
      {
        "service": "LogRocket",
        "domains": [
          "cdn.lr-ingest.com",
          "r.lr-ingest.com",
          "i.logrocket.com",
          "cdn.logrocket.io"
        ]
      }
    ]
  },
  {
    "id": "affiliate-conversion",
    "name": "Affiliate / conversion tracking",
    "description": "Affiliate networks, conversion pixels and performance-marketing click trackers.",
    "tests": [
      {
        "service": "Awin / Zanox",
        "domains": [
          "www.awin1.com",
          "awin1.com",
          "www.zanox-affiliate.de",
          "ad.zanox.com"
        ]
      },
      {
        "service": "Rakuten / LinkShare",
        "domains": [
          "click.linksynergy.com",
          "ad.linksynergy.com",
          "analytics.linksynergy.com"
        ]
      },
      {
        "service": "Tradedoubler",
        "domains": [
          "t.tradedoubler.com",
          "clk.tradedoubler.com",
          "imp.tradedoubler.com",
          "tradedoubler.com"
        ]
      },
      {
        "service": "Webgains / Partnerize",
        "domains": [
          "track.webgains.com",
          "track.webgains.org",
          "prf.hn",
          "t.myvisualiq.net"
        ]
      },
      {
        "service": "CJ / ShareASale / Impact",
        "domains": [
          "www.emjcd.com",
          "www.anrdoezrs.net",
          "shareasale.com",
          "impact.com",
          "trk.impact.com"
        ]
      }
    ]
  },
  {
    "id": "oem-tv-telemetry",
    "name": "OEM / TV / device telemetry",
    "description": "Device, smart-TV and OEM advertising or telemetry endpoints often covered by DNS-level blocklists.",
    "tests": [
      {
        "service": "Xiaomi",
        "domains": [
          "api.ad.xiaomi.com",
          "data.mistat.xiaomi.com",
          "data.mistat.india.xiaomi.com",
          "data.mistat.rus.xiaomi.com",
          "sdkconfig.ad.xiaomi.com",
          "sdkconfig.ad.intl.xiaomi.com",
          "tracking.rus.miui.com"
        ]
      },
      {
        "service": "Samsung",
        "domains": [
          "samsungads.com",
          "smetrics.samsung.com",
          "nmetrics.samsung.com",
          "samsung-com.112.2o7.net",
          "analytics-api.samsunghealthcn.com",
          "log-config.samsungacr.com"
        ]
      },
      {
        "service": "Huawei",
        "domains": [
          "metrics.data.hicloud.com",
          "metrics2.data.hicloud.com",
          "grs.hicloud.com",
          "logservice.hicloud.com",
          "logservice1.hicloud.com",
          "logbak.hicloud.com"
        ]
      },
      {
        "service": "Oppo / Realme / OnePlus",
        "domains": [
          "adsfs.oppomobile.com",
          "adx.ads.oppomobile.com",
          "ck.ads.oppomobile.com",
          "data.ads.oppomobile.com",
          "iot-eu-logser.realme.com",
          "bdapi-ads.realmemobile.com",
          "click.oneplus.cn",
          "open.oneplus.net"
        ]
      },
      {
        "service": "Apple Ads / metrics",
        "domains": [
          "iadsdk.apple.com",
          "api-adservices.apple.com",
          "metrics.icloud.com",
          "metrics.mzstatic.com",
          "books-analytics-events.apple.com",
          "weather-analytics-events.apple.com",
          "notes-analytics-events.apple.com"
        ]
      },
      {
        "service": "Roku / LG / Vizio",
        "domains": [
          "ads.roku.com",
          "p.ads.roku.com",
          "scribe.logs.roku.com",
          "us.info.lgsmartad.com",
          "ad.lgappstv.com",
          "tvinteractive.tv"
        ]
      }
    ]
  },
  {
    "id": "regional-adtech",
    "name": "Regional adtech",
    "description": "Regional ad and tracker services that are common in non-English markets.",
    "tests": [
      {
        "service": "Yandex",
        "domains": [
          "extmaps-api.yandex.net",
          "appmetrica.yandex.ru",
          "adfstat.yandex.ru",
          "metrika.yandex.ru",
          "offerwall.yandex.net",
          "adfox.yandex.ru",
          "mc.yandex.ru"
        ]
      },
      {
        "service": "Baidu",
        "domains": [
          "hm.baidu.com",
          "eclick.baidu.com",
          "cpro.baidu.com",
          "pos.baidu.com",
          "dup.baidustatic.com"
        ]
      },
      {
        "service": "Tencent / Alibaba",
        "domains": [
          "tajs.qq.com",
          "pingjs.qq.com",
          "e.qq.com",
          "acjs.aliyun.com",
          "atanx.alicdn.com"
        ]
      },
      {
        "service": "Kakao / Naver",
        "domains": [
          "adcr.naver.com",
          "wcs.naver.net",
          "adimg3.search.naver.net",
          "display.ad.daum.net",
          "track.tiara.daum.net"
        ]
      },
      {
        "service": "Vietnam ad networks",
        "domains": [
          "adtimaserver.vn",
          "api.adtimaserver.vn",
          "log.adtimaserver.vn",
          "media.adtimaserver.vn",
          "static.adtimaserver.vn",
          "static2.adtimaserver.vn",
          "stream.adtimaserver.vn"
        ]
      },
      {
        "service": "VK / Mail.ru",
        "domains": [
          "top-fwz1.mail.ru",
          "ad.mail.ru",
          "rs.mail.ru",
          "ads.vk.com",
          "vk-ads.com"
        ]
      }
    ]
  },
  {
    "id": "consent-annoyance",
    "name": "Consent / privacy overlays",
    "description": "CMP and consent endpoints commonly targeted by annoyance and privacy filter lists; scored separately from core ad delivery.",
    "tests": [
      {
        "service": "OneTrust",
        "domains": [
          "cdn.cookielaw.org",
          "geolocation.onetrust.com",
          "privacyportal.onetrust.com",
          "cookie-cdn.cookiepro.com"
        ]
      },
      {
        "service": "Cookiebot",
        "domains": [
          "consent.cookiebot.com",
          "consentcdn.cookiebot.com",
          "imgsct.cookiebot.com"
        ]
      },
      {
        "service": "Quantcast Choice",
        "domains": [
          "cmp.quantcast.com",
          "choice.quantcast.com",
          "quantcast.mgr.consensu.org"
        ]
      },
      {
        "service": "Sourcepoint / Didomi",
        "domains": [
          "sourcepoint.mgr.consensu.org",
          "cdn.privacy-mgmt.com",
          "api.privacy-center.org",
          "sdk.privacy-center.org"
        ]
      }
    ]
  }
];
