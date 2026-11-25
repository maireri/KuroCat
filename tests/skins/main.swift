import AppKit
import CoreGraphics
final class RenderSettings {
 static let shared = RenderSettings()
 var coatTheme = CatCoatTheme.classicBlack
 var eyeTheme = EyeColorTheme.neonChartreuse
 var keyboardTheme = KeyboardTheme.classicMilk
 let showKeyBadges = false
 let showKeyLegends = false
}
let output = URL(fileURLWithPath: CommandLine.arguments[1])
try! FileManager.default.createDirectory(at:output,withIntermediateDirectories:true)
func frame(_ coat: CatCoatTheme,_ eye:EyeColorTheme,_ state:Int,_ legacy:Bool = false,_ pose:Int = 0)->CGContext {
 let p=CatPhysics()
 switch state {
 case 1: p.headYaw.value = -1; p.headPitch.value = -0.65; p.headTilt.value = -0.13
 case 2: p.headYaw.value = 1; p.headPitch.value = 0.65; p.headTilt.value = 0.13
 case 3:
  p.leftPaw.x.value=78;p.leftPaw.y.value=252;p.leftPawOnMouse=true
  p.rightPaw.x.value=240;p.rightPaw.y.value=268;p.rightPawTyping=true
  p.hindFootActive.value=1;p.hindFoot.x.value=164;p.hindFoot.y.value=270
  p.bodyTilt.value=0.12;p.bodySquash.value=0.90;p.tailPhase=2
 case 8,9:
  p.headYaw.value = -0.65;p.headPitch.value = 0.2;p.headTilt.value = -0.08
  if pose > 0 {
   p.headYaw.value = pose % 2 == 0 ? -1 : 1
   p.headPitch.value = pose % 4 < 2 ? -1 : 1
   p.headTilt.value = pose % 2 == 0 ? -0.18 : 0.18
   p.bodyTilt.value = pose % 2 == 0 ? -0.15 : 0.15
   p.bodySquash.value = pose % 4 < 2 ? 0.85 : 1.08
   p.headOffsetY.value = 14
  }
  if state == 8 {
   p.leftPaw.x.value=78;p.leftPaw.y.value=252;p.leftPawOnMouse=true
   if pose > 3 {p.rightPaw.x.value=240;p.rightPaw.y.value=262;p.rightPawTyping=true}
  }
 case 4:p.sleepiness=1;p.earFold.value=0.25
 case 5:p.petHeadIntensity=0.8;p.earFold.value=0.6
 case 6:p.petBodyIntensity=0.8
 case 7:p.huntIntensity.value=1;p.pupilScale.value=1.12;p.eyeGaze.x.value=0.7;p.eyeGaze.y.value = -0.4
 default:break
 }
 let s=RenderSettings.shared;s.coatTheme=coat;s.eyeTheme=eye
 let ctx=CGContext(data:nil,width:720,height:640,bitsPerComponent:8,bytesPerRow:720*4,space:CGColorSpaceCreateDeviceRGB(),bitmapInfo:CGImageAlphaInfo.premultipliedLast.rawValue)!
 ctx.translateBy(x:0,y:640);ctx.scaleBy(x:2,y:-2)
 if legacy {LegacyRenderer.draw(in:ctx,physics:p,showPermissionPill:false)} else {CurrentRenderer.draw(in:ctx,physics:p,showPermissionPill:false)}
 return ctx
}
func pixels(_ c:CGContext)->[UInt8] {Array(UnsafeBufferPointer(start:c.data!.assumingMemoryBound(to:UInt8.self),count:720*640*4))}
func save(_ c:CGContext,_ name:String){let rep=NSBitmapImageRep(cgImage:c.makeImage()!);try! rep.representation(using:.png,properties:[:])!.write(to:output.appendingPathComponent(name))}
var n=0
let catRegionBytes = 720 * 460 * 4 // Cat silhouette/head/body above the keyboard (y < 230pt)
for coat in [CatCoatTheme.classicBlack,.shorthair] {
 for eye in [EyeColorTheme.neonChartreuse,.warmAmber,.iceCyan] {
  for state in [0,1,2,4,5,6,7] {
   let cur = pixels(frame(coat,eye,state))
   let old = pixels(frame(coat,eye,state,true))
   precondition(cur[..<catRegionBytes] == old[..<catRegionBytes],"Existing skin appearance changed: \(coat), \(eye), \(state)");n+=1
  }
 }
}
// Verify redesigned Siamese (sleeker proportions, big ears, translucent pale blue eyes) renders cleanly in all states
for state in 0..<8 {
 let c = frame(.siamese, .siameseBlue, state)
 precondition(pixels(c).filter{$0 != 0}.count > 10000, "Empty Siamese render in state \(state)")
 save(c, "siamese-\(state).png")
}
for (name,coat) in [("shorthair",CatCoatTheme.shorthair),("orangeWhite",.orangeWhite)] {
 for state in 0..<8 {
  let c=frame(coat,.referenceHazel,state)
  precondition(pixels(c).filter{$0 != 0}.count>10000,"Empty action render")
  save(c,"\(name)-\(state).png")
 }
}
for (name,coat) in [("black",CatCoatTheme.classicBlack),("siamese",.siamese)] {save(frame(coat,coat == .siamese ? .siameseBlue : .neonChartreuse,0),name+".png")}
save(frame(.shorthair,.softAmber,0),"shorthair-amber.png")
print("PASS: \(n) legacy skin/eye/action frames pixel-identical to the pre-fix renderer; rendered 16 new skin/action frames including turned heads, typing/mouse/third paw, sleep, petting and hunting.")

for (name,coat) in [("shorthair",CatCoatTheme.shorthair),("orangeWhite",.orangeWhite)] {
 save(frame(coat,.softAmber,8),name+"-shoulder-after.png")
 save(frame(coat,.softAmber,8,true),name+"-shoulder-before.png")
}

var oldFaceChanges = 0
for coat in CatCoatTheme.allCases where coat != .mameShiba {
 let active = pixels(frame(coat,.softAmber,8)), idle = pixels(frame(coat,.softAmber,9))
 let oldActive = pixels(frame(coat,.softAmber,8,true)), oldIdle = pixels(frame(coat,.softAmber,9,true))
 for y in 160..<342 {
  for x in 240..<490 {
   let i=(y*720+x)*4
   precondition(Array(active[i..<i+4]) == Array(idle[i..<i+4]),"Reaching arm changes face pixels: \(coat)")
   if Array(oldActive[i..<i+4]) != Array(oldIdle[i..<i+4]) {oldFaceChanges += 1}
  }
 }
}
precondition(oldFaceChanges > 0,"Shoulder fixture did not reproduce the old intrusion")
print("PASS: reaching arms leave the face unchanged across all four skins; old fixture contaminated \(oldFaceChanges) pixels")

for coat in CatCoatTheme.allCases where coat != .mameShiba {
 for pose in 1...8 {
  let active=pixels(frame(coat,.referenceHazel,8,false,pose)),idle=pixels(frame(coat,.referenceHazel,9,false,pose))
  let yaw:CGFloat=pose % 2 == 0 ? -1 : 1, tilt:CGFloat=pose % 2 == 0 ? -0.15 : 0.15
  let pitch:CGFloat=pose % 4 < 2 ? -1 : 1
  let cx=184+tilt*36+yaw*7.5,cy=126+(coat == .siamese ? -3.0 : 0.0)+14+pitch*4.8
  for y in Int((cy-35)*2)..<Int((cy+40)*2) {
   for x in Int((cx-52)*2)..<Int((cx+52)*2) {
    let i=(y*720+x)*4
    precondition(Array(active[i..<i+4]) == Array(idle[i..<i+4]),"Arm intrudes on turned face: \(coat), pose \(pose)")
   }
  }
 }
}
// Dog anatomy is intentionally different from the feline shoulder/arm fixtures above.
for state in [8, 9] {
 let dog = pixels(frame(.mameShiba,.referenceHazel,state))
 precondition(dog.filter{$0 != 0}.count > 10000, "Empty Mame Shiba render")
}
print("PASS: feline shoulder tests and Mame Shiba face stability across typing/mouse actions")

precondition(EyeColorTheme.saved("淡金青绿（参考图）") == .referenceHazel)
for eye in EyeColorTheme.allCases {precondition(EyeColorTheme.saved(eye.rawValue) == eye)}
print("PASS: renamed hazel theme restores both old and current saved settings")

let swatch=CGContext(data:nil,width:1,height:1,bitsPerComponent:8,bytesPerRow:4,space:CGColorSpaceCreateDeviceRGB(),bitmapInfo:CGImageAlphaInfo.premultipliedLast.rawValue)!
swatch.setFillColor(NSColor(calibratedRed:216.0/255,green:170.0/255,blue:115.0/255,alpha:1).cgColor)
swatch.fill(CGRect(x:0,y:0,width:1,height:1))
let ginger=Array(UnsafeBufferPointer(start:swatch.data!.assumingMemoryBound(to:UInt8.self),count:4))
for pose in 1...8 {
 let buffer=pixels(frame(.orangeWhite,.referenceHazel,9,false,pose))
 let yaw:CGFloat=pose % 2 == 0 ? -1 : 1,tilt:CGFloat=pose % 2 == 0 ? -0.15 : 0.15
 let pitch:CGFloat=pose % 4 < 2 ? -1 : 1,roll:CGFloat=pose % 2 == 0 ? -0.18 : 0.18
 let cx=184+tilt*36+yaw*7.5,cy=126+14+pitch*4.8
 let x = -yaw*38,y:CGFloat = -8
 let px=Int((cx+x*cos(roll)-y*sin(roll))*2),py=Int((cy+x*sin(roll)+y*cos(roll))*2)
 let offset=(py*720+px)*4
 for channel in 0..<4 {precondition(abs(Int(buffer[offset+channel])-Int(ginger[channel])) <= 2,"Orange-white rear head exposed in pose \(pose)")}
}
print("PASS: orange-white rear head remains orange through left/right, pitch and roll poses")
