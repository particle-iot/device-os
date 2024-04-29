suite('Ledger')

platform('gen3');

const Particle = require('particle-api-js');

const DEVICE_TO_CLOUD_LEDGER = 'test-device-to-cloud';
const CLOUD_TO_DEVICE_LEDGER = 'test-cloud-to-device';
const ORG_ID = 'particle'; // Set this constant undefined to use the sandbox account

let api;
let auth;
let device;
let deviceId;
let seed;

async function delay(ms) {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

before(function() {
  api = new Particle();
  auth = this.particle.apiClient.token;
  device = this.particle.devices[0];
  deviceId = device.id;
});

test('01_init_ledgers', async function() {
  expect(device.mailBox).to.not.be.empty;
  seed = Number.parseInt(device.mailBox[0].d);
});

test('02_sync_device_to_cloud', async function() {
  await delay(1000);
  const { body: { instance } } = await api.getLedgerInstance({ ledgerName: DEVICE_TO_CLOUD_LEDGER, scopeValue: deviceId, org: ORG_ID, auth });
  expect(instance.data).to.deep.equal({ a: seed + 1 });
});

test('03_update_cloud_to_device', async function() {
  const data = { b: seed + 2 };
  await api.setLedgerInstance({ ledgerName: CLOUD_TO_DEVICE_LEDGER, instance: { data }, scopeValue: deviceId, org: ORG_ID, auth });
});

test('04_validate_cloud_to_device_sync', async function() {
  // See ledger.cpp
});

test('05_sync_device_to_cloud_max_size', async function() {
  await delay(1000);
  const { body: { instance } } = await api.getLedgerInstance({ ledgerName: DEVICE_TO_CLOUD_LEDGER, scopeValue: deviceId, org: ORG_ID, auth });
  const expectedJsonData = '{"3":"5Q50PPeWFBs7A7JdhYun6jd95AyY14yi89VSuwB5Zqx1Q4uZPCBC5iuoLlrbAcDqB1bwlc0_iiUVeWglQ74wrWPeTx1lM3ZNPyg3FrniFCD82i6Vlcls","55":"hzs3HKol0kc1FsQknb8at6Vda5vxG65Sh8xmqqp4biuflPGL_3GmIc0M9Htmk9WJsn0ezPkPElqY_GI9e","X":"ou8","CMsyW9":"_r0tKjaiKKdpwexpcgBtTnQvSYd2vV8j0rFqocP3pTwL92Cw","qixfSDCURWhgSuI7LuASzRPqggPIK3coKcv1ObCTBjGa8xY":"njRKCb2JUi0xSbingqzwoQqhwg8esbDRZAxT4_oSBqb2k2RpgLa2Ziwvhuw","l8WBEdLvb0k3YsfFmaAhjA_WNaQ5fRHkgsX":{"32":"F1a5KDxdV","A":"KRI","VKbg":"jUFti7hmbywuXKu","BE":"8G5_FX","RV":"mEger027QgRLl2voM"},"0Go5q47rR0uqbBcgnJ1R":{"c":"","CEuDg":"apgar28c","rr":"_5XquQw","Njl":{"p":"JS"},"9nEs9r":"kBreODE","s":"N8Rs","V2":"wOeaU2ga","0P":"hR9MQaMVCxx"},"7hhv4k":"RTXJIh16UP2QOzWBXYLYWDKNhxGtabCN3IR1bhScZuU9sAk8v8gRdeSLoFTRG8vdNWl5oUUEH1VSHQOc5RmSObGtM","7hINNqwV":{"X":"kCxOh3eJ","p":"3aRB663r","e":"uvj8x","2EWs":"WSOj","iXZOEEP":"Twm01ijipZwGV","rY":"a74D6Kb"},"hPeUdEqooA12WrNG1SJ0a0xKJcQrK":"zz_QR1f4HQEH0JKvR110I1YROTa4JKM6LW0Ql1vf7BPJrlhM0ZLHI","zI5oqKtu3p4kythQtCyeFfgOLfhmP9MrkVKuxhBMr":"_bouTHlJPlyY_FiTbvpclx4XUP4VDj_ND__E0bmLrxpT1","szfqnLAK47ZeD0YOhCjmn":"wa9SZ98ETz6NTLLDDdqGMiWiXXbEymonpPNMhpi7rUgIDqWEj3RlJGcGixKSueOw0b","gY6EYwhuyaVFQ":"KOaf0nDKlKThHFrTnTLYGJJDOhXznQtAf3rdWbt7o2aYkF35sRCnsKBnysNLS9NHreX9mGKLDMz1ZUSP1","bmI1eWjCV9VMX8ROAmdo":{"g":"l_xN2PdS","d":"SwalXz"},"1e5cMbJvi4fheuIS3Kzcd3L_yeYm1E":"jDCzt1c7yt_k0di8g3ALYbdYTDcxEBWou4h","hONA8j8zmaAwZ":"uSnyxN0pxGjIlV0qUDUUfVlI4fkAKMF_IkCkAP7","wup1SwivSn":"Koeccq33gWv7s2U15mLt9iTJGy__dXE_OJFF6QqiuIDi4YdzQmz","3WF7KK":"oBJcgA1a9XVdRs9hRrsmYwSyEfiMSC8x7rb","e9ghEiFBiemEG3xsiDN5TKx5jrTOhAtK9HTTgl_IWZz":"XYFAcBcwxluQncC4UmWtG2cp5tuUt6Wn6qtylNahQOgMtxiqfE8NqK5RZtMXOs6faINTVtXN","04OeqEwt1eJ6hVGyhXUcRXNYEnxa9uaQ5yYIE2Q0I":"geGZIwSCgL04GPJrMsLfmwXRqaHUhzdnuOI5Aybrd1oe489UpIIxglNZ_VrFslSFllu","nhEIFYmn":"JpgPCGLh6sY2AHKdE1tkGi7A08jn9hECk7u1d15kCet_NngfUSD79CmOlpB4nMgEqIQ4G4c9Ik6o5tRyTXTp1KL8YDnbEDU","9M2fu75PLFohvOctbgV":"QhGSAlagHvxsuCNYRm1ea_x269k0PeCYkQnJT_URrCtOSG40W1G9a6GbhSEf4ywdYXtG4lQvpafadJ2RW0VWDxH9F40oPe","CnG7yPy02":"845ueAasC2_EgvrSRLDVpAz0m0KVA_WQHciYUBDZ2hyh1Sr9mxv7e5oBj15e2VHahRk7AIWcQb8G","mODGZkBQ3JqaDlf7hSK6AlKuugUf":"k5OxhF9uwWRol0w_o3YH4CM0ECzroAVJfliIWB5ncpDsVsPPe0kvNAHLC","Ry7jEpuE5cEKR":"vIh6aGMDKNLLCdmoRTBQfYPznZQrUFt","2kv7SQV3jJDQxrus":"CzjYUwKvUmYjVQL2SxKm9zV_2w2nXCJvYCo98","DzqfyW2":"dNNgc6GXNfas6sDdUWDjqkZ_b3cRu5fjMPPp23aeq","NA9pQzyLD2N9usOx6KN9gVMWEGwsi":"0ye8KkKUKGjCyxnng1T9XW8wGenqSmE93P_zqWmLM13VFUD_0QK7QmUOxKcZtwE9yO8x06oES1nQmJPrzA0nM_9m","FWJs8flTLvgdnSgCiRpZbAKDV16TT":"6VFJ3bxXkRZiOkFHu7_8efUXzg_FM3","APBjvanpH_PM5LqodoLwJlI18z7Fg_5n8k":"VCIj9snR107ymCZS_Q5F_lsvS0PORGexYz8sEgX5jgYpwmkCIljBGoXAaPpKDvxiKJiVa_rKFrX9pS76Bo","GlEo2DnH6P_HZei3cjkPOrzogEwB0KlhsT":"COM0UwFDw7WI58H4ONORm0dyJn47Lx5zgTkzpvU48tPGhLySFyaCVAnv1mv8syOMYt5WQ2_n7QH_","6g92wealsic":"rlxL2gZBxn2tZ1q6S0FJ2C9Ami8t5Fig6g3mFPgFdjw","BypoxRY":"XvTItEoVnYYUXEi9lIadWsso1N6YedARWrutVwlZZpxiR68ryK3GGQySuRll98o00f7ivPbBpjMnexozg5JmfHHtw","agqliBc":"BpLY14rtngZqx_zG3R7Xpa1vAVbgKfy4Q7YTkpbN6BCPhIpQ60NB4nQM1J9S3h8_oVqULrs6YMaRZiy","mxGv3lCy":"9l46UYjaugnBQVBUBhC7MCxkXzaSCSY8eS8GjTKJhn","m":"AFq3Jbh8F_LX__r_rbkTc19YoCmP32nsB","_fi_73618DBX0V9saMKIOEGjV1gEIo8x5D":"NJ28JR4QnHTxiG1adOWp7bigl8M5WemkBWTm9fFyrAVjWTB2fPUy4HIMWmhf1FnnFNGVqxIUqn4JHdNcnH","ngEXJOx8H2gYE":"fWHrzV9Qz64nOjT_r","hHJnBbk7hS_fR4THUDpsdxW0SCVygGYBL":{"g":"eTO4FiYeKtCp","Hec":"36wdP1E","kX":"5TCv"},"XUyqD7JTdX2U6UspgqeYbBu7mz7hayR":"FgfsemPA1zGAkOxL2NwEjvohWuXLAi146ltUAP","kh":"hLhgN_2Jw4uGBifoas1DVCwmxOYeuCjtwvGneBdaW_4g2mHU59mm49","QlfXb8W9K":"THJIbg7TGZ1DrBmNxmpcdKBikQGv3PfEaoXZXk09lCNBRVYCIVJaW0KEMtb0EWzUZcRVxAsFrx4yTkWED3XzSe8bF_gZhFoqQZJ","0DvrVRjUnn8_bVyfzDvb7a":"LRXUYDI4XhrLLA_HuhZXes","y4qrn_6m48cffhVHcmwvULeYCj8E5ACcxeQ49IT":"NFouINkJXj0asqiTno_3g01G6gof10l7Nl943Y7kf8aHNgMI5NN8i2RsPtrigepp8qUwbyRvul","Sy_w3JtyWV":"tlKYPkqnuGRWZOGyMKyVlazgZn46aPtnKKYwT5qbXBFcvRcUF9hVO10YdqQX","TjFWo63TnB87DM3qWueyGGG21FcU0hjA8T":"NgIJjiGnvwizf1qNwC8ctaVkqJxa9cSR5KYKh1DUPAhjfHHR3bXQRRc6fZ1Ix4yYna","qDxwY0WETw_ZAcU":"u_KZyxdx6yxjNYdH5c4ZF1BmrNMhnrpFzHL","pGTDAzc":"VnRPyXID5wrlbWXDr8OpqWGBp8tR6bQdo6tJA20RWPUiSH92oEN","rnME64nDXmNlNxZ8UbCjeRnXLytiBVR":"grKOI4RVbOfpDdgoMinvWuf3tcZhWaO2IYNl2","zyQaxQh":"jRM7gQMmaybBnUF4uXz9ogtVaciyqFcnJAbEaAccBbx","DTRJh5n3":"XMqj1iM6HX4pEMjYhQgBMyvRAp88_IPklU36ztw5ohbEtW1ulTyibtB","_8GLL8KJOYd4mmKBnIqqW":"qFQ82dBFTyyWZwwJWLwX1NIal7b","vq3YYJdakpkpJANvaEgta5GaQW2eCISzrSA4DqOiPSr8jaUBNneaA":"9HpTtQnM074zF1LhOPQv3Mr43judZKqBpmGqgtBdSN9bq81UUFJ0ZpIqrlJuPtxZg","ET4maZ8cI":"UwwafHiliNvNWL_K2f_V4cqYQIujteiOcfGKo4uvEBR0ga0bklqiW_tUO6vlGOzOpHxA3ax_qEQ0i8MyVksEguEpIWVy4eX5e3WEg2AIEh8O","OtA1V8vxNWldU_No8s2VGVGezyY0oiit8gdlT9tiokEID":{"7":"52xZUbo5y","m":"j4m","fm":"QjxAeIGkGJbn0mirl","lQt":"p5li"},"oMXyVf4y_MZ5CqK42LS1TcW":{"0":"","sWCXsa":{"9":"kf8iPOYc"},"kW":"eLoPckKt6"},"mSvzEZQDy8G":{"U":"bLxw8","gjS":"deEN","Cr":"KTzkU"},"5j4":"6O5nnnHQ4T__rbq_CHCekqKDF54","0K22Ruob83aVmNwSmAYFAvUf":{"8":"","I84_Q":"AJtiJGGsvC","Kaq":"7CR5St963b","Q2t":"f0"},"1W2yOXhbVWDGSz":"R8XbjjdN7cJVn8sYnO4lrR6H0Vb2lm60s7SS8GFuPx","sw6o":{"3":"inSXeVmJ3_IvsFiNHK","3Dl":"ihI2","r":"Q8wY3zG4mUES_AG","Md_":"Uw","CKO":"brVp1L","Y5kMl":"RktHdBVedqfhZ","Wr1":"4Jr84"},"tvNSB69lz9LI":"uf9cY3V6xSi6h1iXxK_7Mb","JIfYYfhZhEhJgjWa5lK4tpO11GZO1Wjregpu":{"I":"us1r7w","BVYDT":"qki__oaFF4g","CK0h_dc":"PeR0UIh","cPOP":{"4":"8W9xmD_OPmK"},"wCf":"J_Mk6"},"N_F5ELAI":{"2":"82vJ_MmiFMuj","hsfu":"Ol5nylD"},"w8ERgPVhEh8pkSJ47C":"WdXuzuUWloFxK73V2TgzBi4RhShwmZ8Bjzu_QPxKif2E01Ugc79Y0HqoyU3PfFAaAhvHM7wFUIs0Pi0UT4bDgronOFGps9r_","ZzZi2DAeUKBHdpx6j8iEnNE31jRs1m21l8dCdI05":"iVhTeUGEhzKYEHN6klEgpnQmnrfnFx3WBNi7tPNFdNwilTOMCY","fKNh7F3w8Q":{"L":"5bQD","BFA6Uq":"EYzW8","_uR":"ERF","kQat":"7gzd"},"xOPOOqAENfwh0PiagUwwgrKyvLgU":{"F":"h62","gjX":"TTMPr7DK_5p39W","LKI":"dLdtDY","TUIcH":"tkArmRyIMEm1R","5R":"xb4dTEFshaI"},"Wi":"mjH2ZvQ9SAWFWnAnd4j87uBrOZxRvXzk7jcZE1K2zoOcyO","xe1":"MW2DzubflLLjKZNC4oYqCK9ukr4","GKRlqp3hhIpSin1qNm5uqCBypkTWkICNklRz":"_rm4Vh_JdTcNshzmu_g2bLfYceDEx9EO2b7CCos76qQca5yo7lMzbs_5lHowg","p_CkgfjhrTphWz2jUHVoRJpZoRpbZ813P25TSyQ7l":"TjLVePJ4g0bUeBHKibjHWYC1bn_8M9bFwSHi7brHUmnJTGXJ2J1c3hHKLymwhEboE8tb","vkZshyB2sBSiHd2V1z36tcOGT7":"pGelsP2fXEm2376dHvJtyVjZcTO3g1V6LlPDgq7kcafFjkZgIrSDF8xqAfypE3mRdCovx5CpGmvANwyRtVz2sPjURIDi","YveZQy2i1y64sr":"h4ANk5bVdqY1KjsSYRQndAZmRnGIBJJiPzvigS_lmXrsiOny8t","Tm54A5oB7wp5YtrPI":"TQrNVm5PVfX9HZBKxEb5","KSG81aq":"zMDiuXFhE0M0fbE0SzaUlr_wrsMe0C5S2OhUp5ntiWc5s3vk2CgEyPbC1coyAhu2XbQq1KnrALZUEga_HNzYrKuNdTUezrE2lkVcqcPh0t7cCa3","qWbDdj62li72":"J_h3xGKFCThmrN11FqiIewt0_PgX8t4rjS","6LmU9V6YyLh":"wSffmfxRHilWYGxHylxKIdAUdtz0X1uB","HQqr9Duox06op0yW4nbJf_8AlPjcKau0TaMkUe_2eZOw":{"w":"FrgkhZJeL","vT":"lVv","bkT":"PtM","vs":"0bpZTpQ90LL"},"waCAUcssZCbJlrB1vMV4Iq0Hc05PeIm":"AUwvspv8mm6omKL5kUH49ESCPNiDO6cYqo3HcUZUVesU0bJE9yE4LKjQQiug","IL":"nxDVGcxFBb5Xp9shR_9rfLZ4mRSe9j4e18rfCp2y5yeksGKqeS","y6A7QyO84ArQgfW9uMDDVvPaNC_Bn":{"k":"t8","kFmsrF":{"l":"WG38FAS"},"L":"1Z0b"},"EqvTo":"mloPvs18Jg3OSW_4IcFXPEo3csUnvlCll6y","900f8T1EGLscOOmJ5o7U_DPhrPgszw":"EZL4GjQ7eX8Hx3ZEL1P7TsJTSa7wf4ijy31IvOzJC9v89W6GDKgEzOyDhhuvB5ij","A4nPO42HD5FCQM":{"W":"qmy_wbK7","H68q":{"3":"mw8z"}},"7itel1LgAk":{"9":"udv","Igjq":"7Cu8rwAZ0H","o":"zMvZyF2yJxVQGkm"},"FysBL15P5rcdRblvVgiDOexO9v":{"V":"sWnIUAaRFk","i":{"Y":"1QEF9"},"dbOn":"NxmcPvRHBPQ","KbX":"eLPO"},"u0svBYKlCoZuVha":"9MMdTvwQ7Ko_xVoamPXlHwZnJt7QGk4Fp3kr3XHKmOBQ7qz6rZ5fAfaZRy0mNGZYb_TBylUytcZsFOt2Ej3_","okc2aI6dqa2Ldy90HRczYJQOtc5Mt5BjVDuGx1uf33_tbbrXBl":{"d":"K_XtGMuOU","V3rU":"nKsKjq2JILCz","o":"9d1fxy","Q0P0b":{"Q":"7zu"}},"PVJzK_q":{"D":"dBYv46qSgrQ6E","li0Hmg":"firLcVqEk6hr","cGS":{"g":"z"},"Xt":"e6nymPeIrZV5"},"DQcWcwpPdxxzmb8bUZzaAzA_fwQSjKxKP1iZJHbSBVv9XIa":"bxqnmTrbK8rZ_3wnkxTuQ3NkJrsA7ty1ja0RepIOEQPMGN","7zWbtrF4ba5D0__afHcRfucG":"heegkFLRf_84jyakBJN520l2ef3Hu5v0Z7ALsmm_tAfEA83Dzq1btKe2a4HKnNhqT6x95S5Hjv8B9VTELmecBd","Wab5PjNOvdMwvz2wAlpLhU15Y11wuytokti8V":"An1PDc7EdJfClhn8lyLT61WZwjISju1DISiV3aaePR","K6G":"QIe9fdp4nVihsak5FGHIuplcwYR0DaZoOpoXmY1EIbRBudp95l2OKIRObfyxXFywsRwmR338DV","INjx1q3Fr":"4WSXPRXT6iK0_DnLYngYs3XQXwEAZI","ILy6JI9I38OiWUxp8q7U9yfbwR":"sNLk0GeseNxZ3VINlhKmS5AMrY4kY","rX":"oW3gQzTlYl7G01ZrUuFc3nP6mQTGYxOJ7gD","fwl42HT0WLkmxFv6wBkEWuFhm":"r69NaBgaqgcj5NVouIp29qNrI09","IUu6pjZTVfVjpAvNMtEaUmcm9ejIhpCvoz":"lWUd1Zx5JES15a3VhW24SoaFQ0QhSZmWvz0kiYk4rUXX4C9njImjwfpIAiM6UoW8xDFwNaw7wu3dtn","qix":{"J":"7WUHO9","oBq":"GEmr","QkDl":"XTD"},"XiQurDzR1AVkZlpSUx6qQnalazZWO3uP3f2q2":"xmM2E0S9UZ9UofEogE68L2PTCE2OM8xawoM6gnUeMuIzthPJjPVj7oXO","QRpT1iciV0llxj":"1hOWtrDLj0wKq92JDkaH5n8AhV5","lnmNXj7Rpa_7_pHXiu":"svWFXGgzpZpYM8ebY83FKJbWax7l6ofX5IP","RWGDQMdO119pVtUK2tW":"ehDkVUrT6TTRSefmOcl8Ny3UPqaQ5Ag9hFGnT9Ib6srXKQbyah2","63ScT":"96lFzehn0WBAqN38uB3zom202wgnC_sWWXOpKBWY03DLZWMH0hLs1Gm3n1ETzPoUxuSYmdd9cwAgIie8yt","lCxIX_8":{"F":"cbN","P3m":"Ir_bYnHMRiJe6IB","tz":"4NOJUFoTX7wVxFzdQ","B":"EchAi","eT9mk":"lsxR"},"3HdnGO9hdAuTeZ1rddCp4KpmWFai3Qsniw_S":"RuEOmaky91gKykpRqvk18jd8mh2oh2Al6C8BAyyurwze6sihZ41lXHzJG6jI0AiqKc3j4yvSLsM7","xvvwidRe":"SJjaNvCTIxyitvDAggqAatGD61V","Xd8S":{"N":"uBxOb","rekj":"F1QD","K2E43C":"8Q1Ph"},"c":"maJ4u5JuuYsqrb9RHthPtmLsBxd7SeernxZM9UTAozVXLf3J5fGpx","ncWdQs1PycmO9aKLS4sWs8Fq5NyMPXS16kTIF5tAcLDT":{"G":"WW4WcejW6EowAu","O":"UWDp3GP0jiO","Tub":"Har","fOmi":"cuft"},"GTfRy54WIqK2TBXv_cery0VLDMaIwBnm":"rygDij2kjuKTP5iL0kkt0WE3ZAnwRt29ln_BqpUAO8bmgZPW9gvt","BBibEWDU2bg":"pqi7LpjiPWVrCl5lvEeqDs1eKmjmCNDiyCWXZixlbRAA","dQwyfaaLaDnsvQ":"h3TGACypFS1fzbZlieb9tfxU6EB3tHjfg2CFnoxTaavILZU8u88ssyH0AlMM","5DKZ6vU31ZEZpqbjTCEXJ8HR3UN9sp3a":{"e":"8p8jhrabxAC3BpDqg","8_M8":"XEGYFcqVk","Un":"VYnJt"},"GZzbO":"JkJT8m7e0nUO3_ws1rEKcZyJNp3ysKSh","fVgN6eGcYAcIoLVSBmjzB4IGg":"KoH382bq_cKqVHtxYPcDMfw34p1B00yLuYA8_KhPhpYpjme3b6EnTjFRyXhSXXIQ9uJEFL","UX4pV614JFBAau9hksPu1m":"2OqyrmMnyHXSO4ttTi_ZMQ1T4wAnm","f20Bv7hUa3UqD6AbJ":{"I":"","qOKaT":{"A":"BOh"},"QPm":"ZW","xIBVRoU":"bjFFaWD"},"AtYGk_DywydoUc":"psfgJdUzXC5duQxeIJuKGqQU18b5LIi1c4xBEWTEUZSGbK6XV7W79rdT277EewGj4","0YWWiaNqUPH":"GmSdNIdMzhlpOHnnvL2KpZRu3QqC82qriazFK0xcDy513_B4VcspfufCqWr5_Icm2qJnbW6mqgQ5oTU","eypt0Q9QNf5swpQYxVRg9p37RUKobnhl":"asWhucJV3K8pm1T4XqNmtKtLEVesu8Oasd3O_9judJruYdFUn5o_DvX3AN","r9g5":"dLxYcyk3GfIyC41Xr4QA9uFOuhxFUTLr4QcX_GuS9tdBeN3MmXEEeikTsA2hXAE","WvUDlgdeG3w7HFLnM2QA9P050gmcxAlg5qXqKEOjmBVM":"cueYEbi2B40BlysUBMz3STCB54T3KGPPZSMDWcXlBYvIxaG","CC5IwJuUN3ysOC0mXH":"e89UkSnPOZbKvrwYhFm94whP1zui8mlMPntVij1PhoaSOIxqXHki8E558uaJ5qzN0vBq6e9HAzGzj_JMorOheRX3daS0IaMb_X1c","gIrrkL7paNAn87tsi_7OcSbdOnKXf3_6JbJzsLu":"cOPHtkVx_A_xjVgajXGECiQLaoW1AwnOW5h1Ei42V","NzuhaYV38ApNsFiVsGv0ht":"VmohA5N0K6GYH8ZfLobhZNNOoA3LxBvrPNABvIKIWwWlAIEyas1YqXhapWBb02g9yp9e","vA5eMbpe7WUx5LnTo63EnV3uLPt":"dcg8x3F5BqsibqV1hAtq8tYCiqNzVqpgVLqIfk6B41qMUp_t3","sjnb5":"Uh9P1cxwXoDCo2lZxSSsFNk4o9Ft8BuOuNxk5eBR5K2jvOtLWH4M4qWCEZObnH9_hB4ypJMfzZXnQA4hKkbT","WDpjxnaWosnJIV56KvkdbQvhvg":{"e":"iPA","T":"hoiXLLJ","H":"cUC7b","tF9yyKk":"NMvL0t","bXJS":{"N":"qCbm"}},"U12":"PHQvmi1qJEequDXmooFTQvXlIMbTMUxO8I54ZwgmY6HPmr2z_fw8vGJ13p5n45L","SmSgf7NHfuWpw8w":"IXK8ny3JfODUBA9W","qcCC7_s1E60E3d":"hdCb_eimiSVbgxulY3_L8DV_omg","B":"hJrz1gVLN56AILdf3lN3YX8Y4DeGLxDqUr7AARD2pyPWzXIvNeJ3vNE5ONga","9abW_yvzkS8LssCZlfn8XSDW7xfl9":"YFY_JvAsMPeRL9TB9rQrH4RDieycv639NNqgZT18WMNr3","qy3ishDNcPIhuVAFAj2pvFvD":"xU0nLeMr9fNFkdkzepiAPhCFwkhFrZH7WiGeIm7nz24dWB06cMYTHe4HpYVYdEad0OmN3yl4pQsTjDoBi","cDly2kzxNbAQNL1WwbsjEvt16Mb0XAVQ":{"4":"6cdImM","OmNB5eXi9":"y0t0PEBIw","W0jQrckSB":"mtU7xvKYfE","Hr":"2Iy","qo9p0NzFp":"9iSR3MW2K"},"SFclDPW_ZelO1uoZzJ":"XnJ8LnyV9M2m2ZYX30CuARDykQyVr","Dz2kJmYDKhK7amgBP":"Py4Vg1ESUvsKyD6mD10HIgwI8XIeRUitp_2hhtOzH6Szw49","A3mEfMKA7Lyn":"ijSfLBLbnAXDZh2QlwR78mk55Q67eFUWwpwxauUM","4xseqafP2GHOTpCJdV5":"nySBmcRrAahE912NXjdeySs","H":"COqdTHWEJVQGAirMbiIP50fgH2Dq_wYswQoKymx_emC","66kwhfOInV31":{"1":"QuwuSk","gqOfm":"mWlLjr5UgAu3","cD0rx":{"H":"Xdai"},"eBRIM":"iEV3SI7MohoUG","lMsM":"AgA3zTo4","0o6":"bvrb"},"vTUJLTIQ9POF":"2n_3GUKv59IBZGXLpsJVB5v9ImzPRiqDhLUTP9hTjufRD52LUZQF4C_knQZiP","9zKDIOHcHzQKUD0bJvf2neXczBfAN5g":{"p":"","YD3Y5":{"G":"gbBL"},"Ifaf":{"J":"3Vcc"},"j":{"g":"x"}},"acs6h0RcWSueSFFkBt6lOoGgy6Bgdmmw84ei0sUCLYPg0nBq593fr6":"LkTI31NHduUFLoVqQ0N0GXmCnQ9aNTZZiflSLKzPqiOZuJddliPqnBxT","Nn29n6ud9g3qzxZEr8hXt":"280skMn6G8hURWuWianjL1hmum6GrD","1e0B94QZM0sBwImRp_Aev9B":"8gdBZN7KcUAER0KRp0lQxm3vdOd4m0wNpDBMp4fl6kPPRipkZoLJCb22k","K1XvVlW5qWleSXS4Ykd8xlm4NQlg":"wrWV3rJsoEpxqmVRMsT4J1wC8wuUd8G","DUeh7Xh6cCPyEx":{"0":"nH3hRaRyepy1N","3":"AQp1dGNcH1OR","VUn":"yorW1AUstrcsC","_ypgQ":"hcisis1ahzQ","m":"7ALd","yY8":"FN","A":"TNCvcsa6","w":"OimWBP8TR"},"HfsdfgSqyFq":"nPxqdC26U1bJKkadO0qYc3BvOcOUvLBRR2SoqtF_uIDC1GIrmJVALBujU7ZmY7_R7pO9R0zkrvWF8g_4lJiQJJJHnGjUy83kRqUgEZ","5NK9Oq7s":"O91q4NNHaqCfEyG89ObCqVQK8Biu6QeMX7hAFc8","BmIkGgC21Wff1Jy7aPDc6EuM4xyIBImyjoEovWJd1KEqvorDoxGqTmF9":{"Z":"22FX","_":"ozsMBtl0EjFsPH","Yj":"22x","S":{"B":"bJ3E"},"wiElHOCtqr":"PIUEH0bhm"},"guBGcv2i4BC9zPrXL4QfwqsMcyHbdJBhlWWW":"aPwj9gPsqMaVfOLunzmSh6RSSK9kTPrfPpdEvlSi0rx9ujAi8ykGICIxuF_GYB2k0wlY5SFvOiAGi3ESmlir","GxXldw_p3zNaK":"8wqphcpGCtsbfY_EF1MdO2ek1b","Czp33PoXMg3SrUYrRiXZZQ":"SgaKwu22U_KLHfOHPQqqiigkmfi1CAnxmGWFFbn","MK4jPYD_YlJ2LOjvNg701mge8F9PwyZHewrOxD":{"J":"QgVG6","PXZ":"myZQmSwHdT","Dy":{"Y":"hq46PKjSFO2"}},"IUtSP1kojZLN4Cjc":"OpX2sn4gkWfqPQgJdEV1kHSehUsYpVLEjY89sF","XSpjTsVBvU7V5Qzh4KtQ9ugmAe":{"9":{"k":"LfMRixLNn"},"I":"DQzgyIGcDdtQuz3w","V":"VoMYav"},"HrCBGU":"TLNssQavrHJWy06WaDYPoQrxlxbVlVk5pn09KPhx5pQ","wpC9NU_Qi9AlOXIzJFzCG_ddsfb9FSEU4NT7MjdzGoCl1":"LlkGV3zCPspLc1Z8UGMvM5U2eMm3RthaLiqNYEdpZ9BnUmGfwmXM","4tAWd6Apw9kOP183HpyhCkwjw":{"c":"pJB2ST","rj_QXVNy":"z6qN1wT"},"4He_YmfBbEoe":"ghp13qIeCLBbYwtcS2qz2yr38MdIsDMBp2_qFjyaOwAuJgnTaTSMpRglz5s5SxjLx4nQ4inXbYh_4VdN9BTAkJZAcJfTtS5H70Xz3Ll3","TcFozo_b00gjWtFLeGuv8x2cRLeIZ":{"D":"CanDfNgOeTopboW73","n5Gr":"H67_nRfJ2CXXdXO"},"6ldGBPXofZy2CdS":"seOt26E43su5bLz9PPxXQyW6VZH_6MhrgXGgt0Ry09fwREmPnjkZt3TZ_KHoYU5Ev0F4uyOp24MsnTOG9bItwNYIaqWb22SPP","2dBq599LTEjMwqn_bVDnsbl":{"m":"3ykvO","Ta":"O5_","NCv7yyP":"ty_MsO","qX4dYH":"LWSxtwrzEnQJ_","ZIJ40N":"75CySHP","4vX":"mq"},"UE530Rh0iCD5KZfB1Da3R6o5d_kJ_jzZYMhKix":"euBnogXZiH2CVI9y6J1aJhYHeZF65960qgLiUBsFy9k5JcDOgi","S83vi":{"q":"K6nybV2","bT":"qn2wqa","PR":"WJmC","eBsjl10":"GsNogd","N2T7fvt":{"z":"kIX8Mt"}},"iBjs0PaY":"fmfRG2uE2xujXkVLRy3sjZzjRGPuCtcIkUs1XEZIdG1CaZYbiGNXgeh","DHHOZxS1Ypp_4i":"BMEf8rmiStjI0onsnXxTyv1DN3PyQkrRtqfcmWJ","K":"5QkJ42xOPmT4fyC0mMaudAGXNrSlLx1vit9Lo","EUMvq1DA8TiYIEJgog":"5hubMk4ATctvT7QAsLA_J68Wp_K","XLjDTYgrNnCFA3pxsj5iO69IL1":"MOJejSnjTPeXaVc1QDNq0Z1y3mNUNqQFbs","71IRphF2ZYhSgViSsmM":"wKfUefL4tYJvM3MQ6iclsDPVhxJ9NslsMq8mLJHCi4beTxikPnOKKZpe8UYsTCHBQgxdNVoUqa","Ajhfy6d66rwhYTHKEfWLje7c9ndnE":"dvTz4vaawdyyuE5w9SbsDlovq9RqmhOY02CEQiDuRsAabaGp2VhwMiDafMtwgbzSzg366","J1kOA5LkPh3srPaMxpXrzsk_4_7DP7M9zgeWJLog274a":"Iyt6kTxPcs4LIVsxRG8eE4DcpsWU1W7YQ6tYpiMAZXGVVISNSFclO1hgnxm8MIanTi","MgyJAqc0zJNN99FEK":{"2":"X_MNW","y":"ZhKmGeUImyrJBN"},"IChKfaz5y3gCpZf":{"o":"","AJU":"auqxdXoH9TrZp04","y5":{"K":"IJOIpeXTuNM"}},"qggtJwIvUgwi281MQOlLngJFQwTfgxX8jzaVw19x5FH":"h0t7hwZBzNIAXzszXQ5ocGW7ydNIAipFRFN_DwHO4rw3XbXySuKGtnMdx0hddT5fO4h3Mqi3","oc6OSWlCh1DhT4w3KXq9XBbUYtVG3jYI":"vgN8y9cg31Y_XyMqWJwEJncM6GCx6Y2l3c2UFYTnMMbnlK","qh91t":{"o":"UahsSj","W":"EuaNfPl","Mc":"s0w1IRmHIOLwX"},"fXpCJ0OSxt_iswq":"ezPFYZeb9tj5gTIOzAgcGWiKmue9kQLpsitpNCeJoaidH6J_2kWPc4vzSgL78qmV","VpBFdrd9Fa_svI1Zw_ugMz8uY_jWRAwF48ebap_eBbZ0Kr5":"jagEWLIOb_BOooXaExdLuYRXytsOolwNlun_wakk6AVtj3MI","NJMFRXzJnRerJ2_oOTlYXBtkBLIH3s1KiexeF2ctQZE":"aFiY9d4F9Hkc6u1zyXeNK1mHHaoDt9xkUKiMaseOG7cbDXCvaJNsNxBMg","xlC4A3IniaHTxckWckqYu":"cu8n2boaEZSPHNsCbD3ptzCSE9z5VmeC0fa9Mc1psb9O2ONHQIVMY","q8":{"x":"npRtELf","pnDiwTzI":"_UgRaLY7","mU":"d_lV"},"GNJWS":"VE4xpeUB7lYKsvZgw1qL9WuY6RmkZpsQ70EVpM2OHgKz3EevX0g3FT057SKfXGmii_dAPbo","qlJVTwJScUBeORkvxqj":{"l":"W","Nfj_d":{"x":"ABY"}},"JwXJu":{"2":"","_n":"aTRA","42_S":"OjBb","VcUd":{"j":"YpQ"},"woStvr":{"Y":"Y"},"JtlWJ":"wCc9yx4GGr5Hhs","Lrqngj":"1A60N45"},"QBZLg57t9B5UYX4_rM73xQamhzA00fk_9W0Bh8MTroadsNJ":{"1":"FrCdsiV4S","Q":"W6SBe","gF":"CUYNjXc8LG67XA0Z","X78YzY":"u8UIVkqdG"},"oYYSTvwv4":"2Xp1CoxFPqbP9Z16C_EGBeqGnzcb1xV2LGyQHr0WqgqvpeHNKB9","gZgucMbYJzdWySkdnmhZ":"ELRLPyCo3Qcuc9VIbEzBkZJ5","owlQyNVOtDdm2gBJRESOkc1sJ37W5p0Ul5wA":"qTiR0arALF3EgsINE_jTHKv_S9USU0WMgHWHl0175B089BR5ADtzGxyFCrFNAMHybRMIosFO3os","nHIsjXTU29ghTmr4YY_23Qvi7_oZ":"rTsR03CoTWlA_nU1nggKA7dpiJYHYEHjCV","Q6jWdd":"aysqKdjyjjTvCEMrUx7CEnvt27fWQaWh7s54i6pAA","6pJyMrcEx6_zba2K7W5r4Uw4VgiJ5GmU0MnzQn":{"A":"aS59CDvGVy","Ia":{"v":"G"},"zFCM":"g_2ocSyyyKhCT11","KtwY":"u6ledLbe","L":"n2miWeCH6x"},"94mXtl4IBRDa7bjk_IrGbxlkNWrdkHIXULl0Vas5a2qiMyxqWwD":"tArpL7OETFmH_xVWUd2KhFhsjjCl_CbTuGHLAn4uNXLasB1ETmzJnWO","nNKcAezimxrqcnS5Yl_EmH2JVcNSgVPSDu":"7RqBa5NvnjibQg1DdNL1bCfLIbghXh64phRaaPhI_c","118sEw54Akrrxi6hqQ1rV4iBsqvyjWJ":{"A":"Dci","1aQ4M":{"4":"u3tJ","Y":"3x"},"J":"oXstE0ZDm9mDUlcR","LgM":"zw7wCA0K3t2WJM01"},"kpWmqtlizEuSeO9PyM8rt9Dgerw039NR":"1RQ5_BPW9ykwMuGUEiNiSAMH0BkGHs0MmpgBF9J1CGIrirBuBxE37dBFmMkNjxCI0O6kmLtXhSv9uJb6","yFH_IBeVTBxP":"sXxO4DhFUUU5rjGiPM606gjlfBOdOSWV8TdSsp616HMkPh9nZVOb6tAs7LgKBoNztPUXzTCTdI3epxkz5SpbZiJhRx","9SB":"i4X0o2pOxcR60bQgvFwP3XAdT6Ab7Qk4wcYCNPIesr4B5eGncJ2o2M46fQZsoOvjjAL9mLM0P8ktO3m1bG2WxYVB0hH8X5lNo5od","MXQMMKZHusUzjpPxA2oUCNamswLv":"hHtoIR0DjidCVc07FxIeoHjmqYGmT3_6VJ1r7vOyZZv","DI3WfmSN6JgjTk2YAUKF3hx2_LFrz1xKs71gRNaMywjm1":"ISWUkpCEMKY2_IYx9JJhDQ2COX5sOE5DX14zTJuA4trL41GUQP7Uyy_F0_63cO6llLVdA9sZEI1","SVs":"0HFGx1VB9P8UlJJ3HSEL1dJE5VPLMSowAm","xY4eFkTr3MCyQghYHZ6Wb8Jd_KXbL_":{"V":"rHPULpqx1Kfj","B":"D59zh","HLuE6lFi":"yMNFAXq59t","gbepOY3p":"MuY0sGf","I":"Ad69"},"mswu":"1KXrysO7YEqHIDKEI5zHfUtdg4mhgh3lVkqVSv283zZ6V_wuh6Fg3iO8rfsS3qWd4ett8HQ9cC0","I5c68nNe0OIdUIK7ikZsH":"o2ng4KlJrdsiyVKKKps1JdVaM1g","GTHGtG6K12VQIFY2gc1_5MRJ9P3ZqOlmU94Hhui":"0tor7AbXOfu3a9kg4uKwD1NbrCaJ2iUGNWXfiAzpZ4Kc9XN4l","JOLAt1":{"u":"AOoPW","YmiS":"QXRmiglpZWI","VjQ3s":"NL8X"},"_ysbX":{"i":"4ilqT0gx","rhU":"oeucIe","o6":"X9c"},"clt7qrxC3WPU0ua":"X8uFa3e9cpCinP77uXZQyRg"}';
  expect(instance.data).to.deep.equal(JSON.parse(expectedJsonData));
});

test('06_update_cloud_to_device_large_size', async function() {
  // Data for this test was generated using the ledger test tool:
  // https://github.com/particle-iot/device-os/tree/develop/sc-120056/user/tests/app/ledger_test
  const jsonData = '{"46":"OBTzYnP2zxPJ29OOKMcY8Z22zAt632KVsEmGgWq","69":"3kp_0zyXm3torCeJa0IrQuvppzONM47uQSMpLPnuDmoZp76jSAidStiO4q0pomByDbLgU75ADtAVitFGX_Km","O":"bMzUnYX4Nm0M6hXgDdbyZyqX_rMcinGhHXkiRy81UsI1MAR8G","2KAqBa8mOS1BVklw1s77WAME8H2LO52CEXAQQ1e9A3jBdD2Fv":"yo9Fq1ZDxtnICReiOJGCqdcSwbjnURO78s6R5Rr_U2Lx5JQ3AirrBcKvgd5vkPQXRqDkfC","GUDaD57DUHH3bym4y0RQ":{"F":"Dr0RtPP910dVy5oYy","1KQt":"tOG"},"VcEH37LUkgJFyOP7LuOuJ91OkhkDPHQ4U6NPKRppSKhEO":{"7":"I","tnz":"f9ouSlTUGWotkWV0","DL":"jJJrnQRVOF3","z9":"fsI","0F":"eFHas2gYr7j7xirP","HG6":"Fm"},"p45DuOW4uU":{"6":"OsAzNZMS","C":"0z_F0"},"DujEdh":"nvdu7dniDdV6UOuQrgzxLhfaYsL0BgfhPDHZbaCtG7ns0HPYeDiNukHUA6UKvoVmWNfojPfvYiuLv2","MwWowakyuJ1iLFNXQDdOpCYc6mPKxhPYNy":"Hahwf9oxoz7Xj19gMDoVWCRLWNwwjqlAzHoBmj4rxHIes04OIG5_foXGxfYlx9BuFZYAhJNRI0H1VQ","z4WYjBx2diRtgJUvjtPNtFnw3gTRhHaCss0DDj6":{"A":"0eTvxEkfXXv1RpYNH","x2ft":"PbbXf10zpRi"},"uyiLb":"okinJ3OtzD8PUcqojL8Wr5UTOcI3HMQKdZfltLHwbRrlWcHKA8B8L3","iIlEBySZWzRGx_hkR9BJadKPE8WL3EJKWwU8e2":"X5loWs76qQUpGNx5lxHL0cnbFszAZtr2Wu6m90U4ZCTA7EyhtihutVs2k2_ZW4tVaOdWWmj_b","GN":{"6":"c6hkgom","E":"GAAW5","hlz3tl":"HxsNbTzu","uZ5I9U":"LMm2ECVftcO","Aeoz":"PYDvl7_GeCLnbfiW","iQ":"PqXf","9IieFzZo":"kiy6r2LCd","v0n":"_ZcQ7W"},"6hUibeZ":"WBsslp9XirPPUWFBfJyIiSEvFLtJrKmq","T":"3cf86_7vtfDQ99tzeFLDxD05xqp5z2KqSgMW6leDx7TjKiij40O0JyhZR5XhQiHSq_VwIvHCKKtsxZ0FR1gz3PQbxcFC1thXKZdrBA5","R7VlqJWaX2xtec4EI6dcjJZT_ocCib98Ej7xSTn6":"FFxuOYpJllommFCv53yfQ2i0ipKvLCHr7GlWJnE4QM638s9gW2ZW6ZTNyniZuEr855zJSlIitBBU1","cA":"i4zAKRYFlz7sQ2sDp8FtBUdYXWL8kqTnnPKeFsTTgy7ZPFrIkFIdN805pwj1_RAkfbwfTBFDzDhD98iENWDNX6EhCVzIdUVyqVTt3Bz6S8iRg8PEeB6","dmdg8ix1zDW95":"aTknoyXzCAPQypbyXe7CUnbFlAmY8fnvGktGRV5ZZXWaecQLgBVfpuOzZGEWhEux3","dpWXAoKds":"rPIimrWWxLed1vUl7qOsQVe0jCdoz","eYjI0_zRegKVMjxvtM7ho16v57":"V6eyEEfOEmlxL9fT5HaoSLS9bwtBc9oTvTgsQGyaTFD_0Z7jfGj_ZFwyuxRJwTnrDl32YlODEW","Y2":"YZOfLgfqxzMMtgYMjvaPURq4Omx9cZcYZ404dYZpIQHCwN8orVFZhkMi","ggCKeCiODdW7IP":"ZgZ0vsA0tj_62qjfRauJExL","oh8KHIF":"ImkBFNWBZd1jvdmYLZdeKuLwain5lSSAMFA","xN7kfSdsJfedu8pIHHULbEg3tLjC8Og":"aVIK70R7VqpCfB5WzIZ55xaHwJFLrwdYui9QTlxgHarWMH7tIWwIYMGahJY_xKC7O6yuZVJdL3ctzM18egyH","8gEnFziPePA28":"77C0bxz6KVWaf7cRwemC","0WYHZDvoIpSfVlUhi30rsyChqr":"36FLJ2w2HgePp183rRYQL_eUYAYENVk1hyhOFvQe9EVkZLuIe2CmpYF","nXtCtn9o0KbpFUF":"oKgYbZbIb9BBkv1jFsHUNPDAvuf","Q71ZfenN36wiX_WAvP":"2jj0mcZIdaH5XVDyK6t3TGrfYtqbSvBsCugF5J3SqSpc74Wp4pexb3gLij5YBmm2lI0oYE7UcowXcyE38ZTVhXY","1DdHGAlCOMOlJytFcEQ5qTCu17hvDbwv8z4Z6MYI":{"i":"IqSlBGj5LG4IC","M2":"4oG3JdGhEafKl7em","O":"uxtn","ZClrv":"eYu7","RAm":"eS06"},"F016Fw1AXffdy4YgjZ9SjJ4lJrv8YM4ODecLWg4":{"l":"_HmD6","f":"i6Wck2NxL","Pj":"Ga3BUg","4iPwZ":"5Ger"},"19OxSuhQ21UjR6ImRFJSjHZkxE1s7uonW5VpQaS":{"Z":"mL9L1","LLb":{"w":"7AO"},"d":"U2gFrKpO","m":"uTrumgy6bQI4P9kbD1","axY":"tR6r"},"dPWOUJSD0fQC07iQu8KHdwMMnzzQyVSLV":"ODMFvIVMZXxHMVWMhUuFliCWy5PTdYlVYgeLy_u6QAPker","DZVIBIqObBdQg5T2x56lo9bH3NFXnh2wWf5PE2Pa":"4c5vK2z1NX44paRBHDKJWpmDXbIs5AWn8CM4O4AIZnS12JFj2","oux8O4":{"d":"CxKI2YVloLN0ehI5FqCKx","87lJ":"a1N1L8W6hF","z2k2":"g26rBH"},"qD8RTz":{"9":"mS6ywKAI1VXvw","B":"MFbonnFovR4ItIFp","gMey":{"3":"9dttLNFQ"},"x7L":{"t":"I0gzV7kwe"},"Yxo":"cpIOO5Z4Xd","mh":{"9":"Vazp"},"iCTTy":"znt2J3Tiu"},"Pxp":"W821_pcdTlhYq9jR91yvxSCxFPJHg3EDWzXRaT8MeqmiNSGx8uDXDSlXB9efXTO2p4E","y9kYlQcv2":"XkqsjBsH4suhILm_XuYHm7ekXg9di80WWgAX2vxca58iRPix0Kk2JEIwnNgn0rBg5UDDyLyYpmkVOnTO2xV7CMLQjmUvRkKdbzLcaa3Og8Ig","mhC8psN19kdWWZnwZSagzyGAPO":"ate_QPmBM3lugp32rxuvMCmVwW7ZLEkOqbUeQaVJIZin9iCYCwlyLyukoepkxKJRHPLBkTkH_mpvp7ySUJyUjA0cp","NPaCc145JL86":"7IRA2Au_Z953rqA98eZeO9pUiQj6zL7EIQkyIRRYp67WOxR4TYOGAcjYk__gWcxAjw6G4kHPE92B0o96vsTirZSN","I0GXtjWBimbBS":"9F7jhgXrrIPPmUB0mJ6VhObXiOT2aiEeThAckBIDLl","M2HClApM46GWjWmHMrq03M1LMBTOiTKFxK1JERnLpZC":"dK7zLNlbJBF0S992518fvJgjpwnU1W3K5UcCCW_IZkFHBd_8Vv7IkcLp_Y6In4B45MECpx","oJ_FeJ8ny6kSsltdxX5xttVk":"6CN1Gpmtznm3_o3mmRvbbp76hgttnO7Nhtlko98oHK7iFBayD","wrSPtdK1hpsuIt5R_5GXxThV0spmWreNfz2azW0PFMKMTCzYFz":"6rBOJZw1tx21wKCb7YqxOIssfL6HefGMvyr6S1e7xJqCdYr1OWHlhxq6G3RT8S8cJ9R9L","i6VUQkD8jPZAPHjKI4ZJqOCt":"vwU0TMmkoxXSIJjebPnZrhVKTSmgHyQmHQ1TDicZoKcc2hiTTdngxSG7qAv5lg","ryj":"zRoqyomGFiwNgP2eJP0uygQGR6RhEFGUH3LbGfYJMmWMs0NtacE5zlYOaiGRQVQgKiVBkLiCVe8GDXY0","ojOe84WP0Ed1K4":"wpcix6CFDXxISNNeO0M_g69OWG3","rtvR0f49qcaKN_TKjJ99fWB8J":"tg25wNeleqLLFOl_SMesjj3Q0mqhnKQ7kvTlzDrqhyWFDZ8o0","hobFjn5KgFwMD":{"I":"e4KFUvFVsc","0_YJ":"DRWBufC7JVGOW","equ":"Z7VgoOJtRES","C":"N2wn8OIWgQ"},"I":"qqpjZlM8LEiaUMLK6CcY7Oz5QdUJ9jMS55PDrCIdJuCqQoZUKEj","Zo6P":"0dpMzBdXhPP4RSeDfnHuUYsO3vL_vTdiTniVRU","FPbilZ1PYpMzb0EP":"nU2jK5b2geCnTnVVcBI4DdbdwtYARwR85tgROygG0y85No0wSK0RIqA5mjJkGsQnBObOJ","hag9xcNbZNN8Wm7nN0qHzPFLGPzx":{"A":"IaGVp","8Bm6KXh":{"3":"Ax"},"fTcJU":"9GHZij4C","7r3PYc":{"h":"TUB9tD_n"}},"bVL8Qa4LRpBAvTHl_cRNrwh6XrROaG":"u6JO9mVP5Qc626Un6UXpQ9Y6VGNPdAXIOCGo9Ds","AwSTZ3ORJGyhCacXCqyF1EWf4QlsbPPSFf6NIW":"AaGr24PpKYjXul2wyTpJbBunFt6hvybj2TkCZHJL","F0b5qNi2tDDPIzIIPQe":{"Q":"6DAxa3","vbzMr":"ZKYFYo","l2":"adjJ0OK4tMh","8e6":"naFH1dZ","e7mbA":"YJoC94GF"},"apWyvKVEtyCbBaqDrq3":"JmjCaazXgWTQduQHR1Ogo4a1YsiMa0jluL2jUHP5ZBBdXllvijwP","kLnngp083ZnnCeb35CebDcw506hxrvpZ3X0dCW_oXkFGPrnFgj96r_v":{"G":"GnDH2iyTpxGR16BJNUW8","GWB5MC":"iMBBXFzWe7cak","B2Js":"ZcVg3NqfmGkr"},"pGb0NM":{"G":"N02A3Xfh","c":"HXtXJteAlIqqd","J":{"5":"","wo":{"X":"Tr"}}},"GYXZmFy9lreNkvl7KDlNqIlJTb8o9P":"eK67KCMf5vNbVJSkiZ6yIjfS6T1BS7Te5iDB5vTFjNr9y43tWqpVSuU5UMhLQxo4_UcipgK8uxmzjIcWg","1148yy2kzB8pgJdWWECkufJ":"XqZ4IQYIoEN1QSaxY3pe47UR3JP5S4ji_wZI","ne78b8ki":"QPyZUhzFvP3oc8tdnBr6aypkKjegfpqxbU9YamNWtf3rHuW0pCWpPq449_bf4Cma_HGflvw7PZcMyERaRH","qiK2BpSQJMah5k9hjHWFcx6T_8KEA4VywfPWPGcFtX":"rOpYC_BfojQnHpeiE6Kme8Kq3azHiCJEKvj2XbQ2soEWbxNURG4vCRmW8XY5xI","Kggw8nGRDw6vISSPbOK9CmEB0l5k3Ih":"nJnRWpuQ_amJCkK6mjTZX5KfM6Og9SJkYEKfnOa_D7i9Y","4ut33F":{"U":"ptZe","QZW":"yI9P","Vb":"P2EHghhDCZVW"},"YZTJ8CCCk5kQWe":"phIOoHH4rUnrbU6v8C3BNMBsErpZ","rkUzc2Fmk_utRiGxQjrCH60HymznB":"llAHP8fb8DyLYhHQnVROrlgL2elDCVCdPJOqpDEPPu2Z2yVJG2zPri9onKwxECiHyMzozA","p14rnqkXtvq8J88yE":"dlvRutouu7kc1lEc9gwbEtg","z3g_yIqi9M_TwKZuAJc":{"u":"MbX","xn":"ZtfKLJeC6CH2m"},"NG1rMiZ3updX8":"AvxddDBWzUrJscfK0mJA25Xu0xbFrcLdIrj_y65SUEjfCer3CBtqIVzkL37zjmSWeFBtPbgDtsJtPUwbMOw0auNYaD08","oCTcpxehn8znwG7DlztmuTU107oXhyGQAC":"rxHQ9R4nE6PqnVhTOUkmbJAbthFNEvd7_7VhKh_bPFAnfHeNZYd3Pf","_LsTIClVQR0kHyHY0_fYYwa8Fu2IdMdPd1":"XJsl7GqwvvOj7dGeKMFQVzHQz_rRU93szcOYwp1GHm7F9QfjPY9xwMTVks7_wEQs0w","w8ia":{"r":"Z8","mR":"Qr5Y3KC","FXwAht3e":"limTqRFC"},"ndfEZXRINmZbHDjkV2qsK8oXnAZ89cRcVslKs8LDBBLPGy3vN":"RzWRDGozBgFDx4ALwYT4rFuObpNUjvPItjTU2kG5k7Itxx2hIYRVY5l8ev03Ndm0x","tVaRC2skgo9eET1VzRGmWuKP":"777wUT0z6bVs3jh9fZQ6ecyEZxfoetTgZeQQZHiN","dboCsyfUaM6K":"bBpnHYJ4D0cswNx1wrF5_sJT7kn5DoNjsm3f2IP2dlTDuxCPDLSmiH_oFXqk9diSjDN","n213fh2g":"WRxsoklrN04_KVH4Hh6W1QhDLvL65l3xdhxcDO9D8q2IwAX5EOJBbz9u4ZFLC3ibx7_FLXdJog75A8I7HRKGyFrUU1R","q9nn3T1HF8lidcM":"8u_b6qsQdp26ZOjDky8ar6XFq4ZFZxyYxk7EcRITAEH","blnJwntMPGnWMdhwLbcOGZYR":{"o":"u3","Ebl8":"H6Z15Rh","_a":"L9P6","DBRwtu9v":"pQK0tQX","fNFq":"vzfd9wt74xH","218YN":"cYiKSv6"},"AR5r_2WCHne":"oO2L4krA1POcKY7LTL0","PWBxUVUEQ_zGZ9VcFdIBUDBPyXfQxHYJh5":"RCW7Vy8HY0jHHaWPn9BMrzQvl1ficnqVDaeDLv1zwcpo9f4bqd9qshF2qBUhlvb2F9mK4zaYp_eRSDVF1uJ","2FHl":{"3":"","oEG":"SYFd","UH0h":"aCh","iosu":{"6":"mW"}},"2v0":"IZ8shBfRlQkJUENLxA7416za5oihVakKcVkzRBqS4UYyd80LUJ3lE","YbazoydrCdkE2gUj3uZgvCElQoRwDdL2YLMfxikt5pkA8T":"zsqCp9zPlTBIJSqhsNUvT_etTbSKRzWZcLwS1VsEwNOkrxrCjwSuxP69","bMDB_MnxZFwhWJFREwIR":{"2":"UcYwfuIjx","8JFg":"5xBiUgoKZp9y6R2E"},"G":"j6YPfOYWJgqPVypHt3Mg8htktO9cs6GHM0dWnJKaQQLeDxFcl9BSnY60","XKtgO52R8wAeZMb9_":{"F":"CEhEP_","kp":"lwxfJmBL","jw":{"U":"M61qDay"},"U":"l8dC","tbe":"jYsS","fcA":"1VRIA"},"qY7rtQNC8KTzGIzLXKz":{"m":"hrhXDpDrVrQrLBYTKJ","Kt":"XMy5T","V5Hn":"zUm","lzpDKKJ":"POQb2sxUWQj"},"HHqTS2K8VSqp":"s01e7kFnvSkl_ofI4cZHreVGRcGjFcy7iPodQsFTK_sAb3Izrqa3do1T","66_ybhdfBLgT1zF0v2peVMjInR56ugDNK6luMvg4iTluD":{"Q":"4kdoHeTLh","kJ9":"chc","q3U4oYn":"JxBfOX2F","DpWx":"f5Ee","k":"oIMQU9mlvCFY"},"T06DaHG1":"E6sgvy6JZxukPop87qibOPGr1EtzFfhM3I18V8SUcuCqOWQFCImnYry0zvzbQb5WJkn1gQq","1JtP5LJ5P_7_YRB":{"T":"","mDA0zonV":"hIUfsUbT","10w6Y9vUu":{"b":"XsUR"},"b":"8AN3K9bHOoL","X":{"T":"UOKg4euCHM"}},"81HYbEMr":"NsvS2nnIOso_SSCVQvbzqFeHB5NUd2scGKMqkWeV6NBdNqVZT45H0DGNZgjceSGWP5uxyjH3OJr172QG","kjdneQ65":"VSQKkMq2udURt87ZoShXvuQUlRjUbDJqqQ_By68ci7cZyA5A","1Ze_Nj9v__lFYetip9GABicWgGcnOh":"OEd_46GwDv4Uk4xQNx3XctCN2P1aby6DkHMS252atF3MukS8MJJfT4","X04DLdlnOFDfvj6G8M_":"Fng07iblAoeWPVFmvGATys","p1KEnODJ":"H5EmKCFvMLxbV2jThT4AWmgW3nd2q4tDYjTWl1VWrzYJa1kfi6eXQMu","HWKb1rUyNUaeyqEgUrqXktR4L":"zOBPz0X7eLmn8XYRieRVg5vLqHt","JiPOmrzXhEr_sKCEzT3uysnMZskEvwxa2sws":{"8":"OJnLR","lh1W":"zj897","0J":"WpCj","k_Jo":"c5wmgG3BvMr"},"jB_oA0wrKAMl435YNtgtBnw_":"lCZHzuYhv5sjqCPpYr6oqvLqO","I2AyEKuNB":"ihfWAQ3VVe9C8zfrRCrOOB5O1hM_DTxwP3ovrdF2y_ca14o22rtUePUqGlQCOazpLdrY","jqPGST7U8Y9aTO5x67q4zHrL":"Teyr7quG1ErzN5hd3ujtwrJM3ByfrTrHLl7ek_Ufame8BCqcfYqbTyN","ESml2vXmSb4e":"a2ckf2Xl0zddcmhxszWDwxiNZF3PfN8bfLEaXl7amX6c6FFE_gIaaOB7_GLEC","uOTRG_3BT4jN64pHv_UWo":{"D":"Sqyme6am","fw":"wcEQJs_v_","a6m5":"oiT4LX1","c1":"uIrrvc37TrsHPbW"},"QAe_VtmwehOs25mpfglY8IYEVu":"TXT7fZH7ZAiM_yQw4E3kffg5h2OWP7E1eZ","33fPcCx9TaK8Eg":{"m":"qIPNMkeIlNaF","MWk":"QZ","YJnZ":"n2J","Nf6":"Bgr","s6Qg":"exYMRMlxAPY"},"HeGwR":"cD320EK8ep77FoRgCnnifGHZjOLpP2rRV15ca1P7CDKAdfeBZS5rhA4lH0mTx_oHrZvLI","2QxFMF8":{"y":"Z5","k0otwm62y":"2ytxOwBqP","d59feK":"1yzApFePqZq6Lz","bLo":"J5","hVKn":"n1M","PxCLEse":"c6uWEn10qDq","xlxL":"nJeVMUwkUN"},"pygrK_JFlDfMWsTPppW7L_ndhaQMkIgroY":{"l":"tl","Q":"5XFWeym9UB8","XYioZ":"thFpoQjMRL6Hjq","R":{"C":"iqk"},"0235KAXL":"etSdS4MLF","mow":"Cyz648HADKF"},"xqHC_0ajSthAxykJFno5WLt":"nGbjQfJE5Dg1_cGwZzc4kZbKSnn0akugZNEeD","HKsuCr_69jmZLT4Yw2ZfV6i5GRgyqDKuCVcFMH21Axyn05AiGRG":"Ydh2NhcjNZangFeN2tXqz4q_HJViJtXwT4QwvP2HvM82vaQBTjJ4AHnefspCrtbbhP","DJOTZRTy3jSzHD4Sfsimo8tXr3Zu":"PhsnJCIBaso0zfzpZXSqy3pMr7IDq8RoNRMBQHRlMHWi08qfgxrLWM5ezwl6XG2Z7bD","ItaTa5YiovAzutY3":"IJfgS2RepHwZjlYnuNpufkrzleu96OmHsZNXMjl8xq3yE4fjDeNTdsyG8we1L1mQBqNuz8sZ","V1P":"8JivS4KMTSnXo83ag9fiD3LBDNMor2lSqDpNsh60Axqkhg1","nY0d0UztS2hHVJwOQL9yGLw1Wf3U6s7I1TzccR22rBrhFZ7":"g9DHTHYbCJpP48VsbOsippttTufLt2WISwjBl3leIUDCjy_mVYVNddfhX0VJKd","_vzwR5n":"UNrRMThFM6eZZflX6w1Q_Ie0yo_eEFLJJu7341yyI5dYL0Fbfh5jFhqNlnROa7S0df1F4OGfKVsMV67Rk9lYy8VD0","IByFvcHM":"xEqFRSBTr7NkS3Mov0gwkFUJ3PtLElZ","Y5cdfD0kTfFoqM":"ikgRv_bQTvuGz0pEc_bTfXyt9Zt9seYtnunC9U8LT72iH36KaTxB86eV5DKKotmccNymcNW2Vv3UFlmsHh","61_aLIVdZrjTQfRUA0":"8UqQwR5p3X2uzKI7NNeIlyqwrlS1LhBOrmOZSUu3G0","iW4mIQ5xI_uy231hwfbPISR2Jp0aW":{"z":"x3zp1trejXEk9q3y","QIE7Bgc":"aPf6yf3"},"bAybF6GRnYGilebC":{"J":"Sj","YyK6":"VOfo5Vlha2Cx7u","8kyh":"6bxm","aQJ":"37","f84":"7G9oza","S":"YHIxQckm9dwwh","1Oy88":"l90fR2"},"ew":"bCKrBngZE1jiib1g4UkiOn4oSeVX_","ReP2QMVCJfswYVM01YWjPJce2":"noEyp86EtIaSTYTKWZyDR3vMBr11mEHrlCMNfptGN","87nxy5hT8EPMEuaizkJ9zPG9Cd":"ZrkuiFXwSCJLpsp6hN9AAxXv_FtmLRcOUJmzWVNBFU0h7uy_uWU8GG4YkU1gq2d722tS5Fddl1PVHbp","vrzA_nkd8Xxk4wO":"p3qOtwSHnSI3sktm36HXghWeLekRQ72HsoXfT","L28":"z5zOyYXrY7R7RpsjGDjoa83K2DJk5pHwaCKadfmOkd6c1uQKdSYJ51dmSHSzpciGeOhUHIp1DXB1B4EoaPWLnTniK0QShP9jHKdJ617lCVsIEIbT1A","Ie36oOLI8pox":{"b":"zem77","TzCVOx3":"fpgFGN2o","yO5YEh0v5R":"2z79xP8LC9","1_":{"_":"iF9ubw"},"ncu2yeX":"jsnfX6","uE":"Udj","NmfUhBkuz":"MZptwbnMN"},"OZ6yZQztQsGUqqaEaLuUM_Vn6SghkFYwcl":"I4Jl89enXc3o3pPizjPtOPuj3nwJctYIq_mmqTRWe7CAcusJPj","Cg":{"z":"","c":"8UDPIpsm9SQmL","ZvJ1g":"0aQml"},"yrFqcqO":"qBhKYHH43y4ImHeLrUl97XJqdJhrsiruDT3mt2nJ1_K9rllri4oy19welj","kmvwXLQT9ZdIYBqmU7iZPRh1JDf":"JFx3nFsQ4Abzb0aC7i847KW2VRdt808oadVDh8QeJPPUlaYV5_ESReA5U8nQQ2VP3al_","2iQqvSHgTMxvVGRjbC":"5pefO7ravivliutO3BthwLjBGW2X1yKSwI54mVpdEhUCh5xxR4aXakkuhWJJVjy43LBXB2JCo0HT","kT2bRFQPa_wuCFqkKD5uKFxCY":"7ISERyoJ8qHaNgnyiNgvGQPj8AgJiCZSbVbSMygaFxzJIkwVnYYzcche2OnWPL","D_NnbXf9KeVdp3vr8ee8":"j4q5FJu0oqhOkoYlIilxiS9t69FIT4NIsDc","jbyD9wa5jG_SLGIhIrymkphVtaWXmg":"NHw86J3gcrH3oV3VvbGCz35hSmJo0p6J8IK","54EcRes0d42Kvl4PQy5YMeZiBA6y487YS":"NyH_VxIXbjtdBgbPW0avUo27yEhOLMH8l3lp7uuUgy05oZOpwqPHIZe8ymTJdzG","0D_FPgdry0vajFeDGWiLZyntaEbmI30YXopTKTVsl3639":"S7CmCkxlzQP9i7QXAVuXTGjLZHdGsHLrup5gaRd5uBUJdpU","T1hXMxPwU0UUkCR7zJNQGZSrg7Q":"7jhvUcCHyyLxzSGiXZQsH1zg5OzqZCo_DygMGe9mk6PrUsbWuJonDl029hEkXySa77nNltaDYSBOm","8kj7w_Y6k":"_TQFkw9HguuAdZvl4bgObZjzw7LRUsXiUKsptcFzYNrfydyIn027cAVrjFD1UTFmu5IBOr2hCgm396dTvAFAE8LwKI8DnhmxrbBI6XAQKm5QP","yqDIpm0Quk_JwXK4HT0":"frXd2fVsfkJG5IFqlpdakn9RVlBhyHCaa_Mg7TtGx","KjdYK5NM":"tMfvEBFdnl8fyt9raJkFUXFOXhesvqgqRz3Ol5u2GL89mnN3KCSfYkFckVLc1N","Fy9LtOMmMnBYE":{"B":"YagXqoARK6","VP":"qWLZ1MI2","pnvpN":{"U":"Nr"},"rohE0":"cvUmC","Pbhw":"74t7hr","TJ":"3Mk"},"Ak":"LXy64JKyBKF_1IZE_A8L0FkTsYl3","hT3lSx":"fqi3Rmp5sz2AX42CKd2gkh1JK_6vO6","U":{"1":"vlnCLx0","U7K":"v7L","qPa1t8":"cCSjg1zw8","UoC":"yM","_":"YWK1","rb":"fuFb","GPw":"oc","O":"yeYh"},"zlhMkSSf0OWwv5S6d5vqJ_2ShLM4IoH0fxh2LwSOJ9lr8":{"n":"qZ7ZZr","ig":"6bkEukq1MYUFV","Ld":"XHuvT","A":"1RjPg59pSUo4Y"},"MvLalx2Q5QpaGaVX_j0Z9YNm9p1mRcWESaMWNKmhxc1EAjCHaLY5fqB":{"F":"8HydFS","H":"WlloQtFoa","Eq6OH0G":"QmELTDcP0e","kpYZs":"9kHXRZTdkeB9AI"},"ylVhbdvL0sQC5HQrhXAtjL":"JMr1jghNCanFi5ZsvECnjqQiDXZ67YK2FX8tzaT5rI6AjTN_5oeKHShKE0tE_H8mQOYGhFtA9xIqacsleT1TTlLx7I7XO7VH","f4I6Om3FplPn7":"iGL7ogCSbbxTjdl02pYQIX2I","KGNC9Dq8dIx0af5IghQ":{"1":"F","Ax65":{"_":"XjEVVA5so"},"mqgfP":"AOugXkZ5k","vMEh":"oep","wT":"WwWr"},"uEYREjr0h2lo8c2ZKStWh":"r54I4vAJqknpcR5Mj1MYNXyhDvqZvcXy5GVPNvlJRM21Q2NN","wP8u92t3OqycRXA2z8KYZAY3QGpWALnz":"ANRwBIpp2vOmUzMvQKOpqFR1O0B1GixBaXbVZGGpZ5weJKjHcqxBN_qm4yEcrPANPT7Jc_1Vgdg9lyt","tInE1DM7HOAPR9KOeb7KCwX0B1hSb3Eup":"sx4h9csuXOxBIa91pNxPfpreUzrSFNQPuJpgzlTEF9LNXT","HtyX4DbWp":"rnwFENmQJGZhIIdQEroOKpne7XytLMBpRJRhz0BjadvQaRdVrGNSpkjimHf9GNIJtO8xsK196K8AqgK7Ze","TibA":{"B":"ZwA1nRp","dg4G5pS":"sYmsltfVoS","4vKmmp":"p7JHYXZiTZ"},"C5eIXCb1tGc4ctZNoENdy9C":{"W":"n0YZT7","e0d":"QcGD","G":"SdL5d","bal":{"r":"yPwcztqo"},"gBTdB":{"s":"A"},"BmE":{"2":"JDVMkYaQft2IK"}},"sfXF9mlmMBwcRTA5":{"T":"rJz","Vc9E":"lGZ2","R":"gDdTv","H":"zlf7qT8qy8j","tT":"XYhwObr8Wdeg_"},"7aGNrSr0hDzjta":{"6":"y05","M3kf":"AbmDRlOzyM_","BALUSw":"WCDnnB"},"s9a":"eke1zjEeYlfOXqrLZAJxDdxP30BWKgYFz6lAXj_BKzs6bDjGokswyek","PWayAIYXOChAp7D3E":"DQwoBE6h8iLMDJnbVECxTRfpNMUCELDRpLsm_DWt7qdlXa9Bq","2nYg2cLJCKsIhTZ0u9O":"u99dR9KslyZ3mXGUI444GDZO9qLKAyblthr0","J_k1S2ngLraQNSQXzlHVozWCnvc6F9r":{"d":"PlYR","4e6":"KW","V_KGdCH":"Ld5JKD4"},"fTzgtSV0tGqOrGfC6FKqMWziuo32DxZxQ":"NCGC4p6ecPYEOLsoaAUEa8z0KvI3oGWH4A_So4KNgswW5vD6bqXkkB9suVLZsIcndhVSQkvbqQ","k3zOvuzp":"g5wwXBo6HI65v7sOkV8LQJMbLIJp89D","OtR_HeRQTb4llQZavLuycSdb3e9KjM0rwqY":"PvpFSsZkyeUv_kz2Uh_CM9sAOhiil0MmavNo7_sX2m3iKg41oHiFhkYYTFcluw_LAwr1DS_j1SJdIbRVKRt3K","uMVMda":{"2":"pv6xsGTlNqu","d":"G3TPx","BVA":"QQ9Q","Hj":"24f76PPoEpRgXd","K":"TgNFPmvyW","txprr4aiA":"F4MB2ruTU","so1":"cvzmlZlz"},"q":"Rb1pTFLKCxtLm2wLmKsQGB_PM3CZPGjUD6Mu9HOBzcsei_0BN_MUO3nEmfpYZKVhWdIwIps8pHKE","TGeNqVSII8":"EGpAgOkxcbz1MIUjY7kzC","7wn4DVB0":"AydKmTs1JetYLXy9xgsnBLkfQF7SmznQESluJLh3KD","6c":"Q4jO7qHLoSfgEchtQGi0tA9jw1JsE8Y1DjJa","YMpVA6FXxtMjjdXqljBf_6jVqU5":"vus2PPTqYRFRzpbS6iRKYqzp8YZl8wepYzbhZojMI4qsKNour2DSq4Eok3A7O4lRXFxE7AQ2yccQTK","ltDdHTwwO1DjWYfN3cHntniBtJEppYtQIb":"UkFqjgDcqadsah7ncz8uLuH9Rq0sf5kjCwknRFO01uoeSt4JvwvnVr8x6hfKmxPwiv58aA5o74A","wKLoDsvP7gjO":"NiekUYJ7GvGi7JRXoi6uzHFM9YeXDOhpR9y9DAf9LL1grINoBrzgE0Xds8SX54r7ELaB2JE5ulp4t7gxn0f1hnQZLSJLuzqV6GvsLU5YuH","pTod4HDiL":"YM9PQaZEbAnJYbtMT3IhpnGbx0DeC69s","dxXMKM7A7upxVcz":"4ob_DyigaHH7hNuxccjBfL8R_BtevYFzZ18niBCM0JZdaa2q53"}';
  await api.setLedgerInstance({ ledgerName: CLOUD_TO_DEVICE_LEDGER, instance: { data: JSON.parse(jsonData) }, scopeValue: deviceId, org: ORG_ID, auth });
});

test('07_validate_cloud_to_device_sync_large_size', async function() {
  // See ledger.cpp
});
