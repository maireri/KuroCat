import AppKit
import CoreGraphics
var testEyeClosure: CGFloat = 0
let physics = CatPhysics()
let size = 128
func render(_ original: Bool, _ coat: CatCoatTheme, _ closure: CGFloat, _ left: Bool, _ theme: EyeColorTheme = .neonChartreuse) -> [UInt8] {
    testEyeClosure = closure
    let ctx = CGContext(data:nil,width:size,height:size,bitsPerComponent:8,bytesPerRow:size*4,space:CGColorSpaceCreateDeviceRGB(),bitmapInfo:CGImageAlphaInfo.premultipliedLast.rawValue)!
    // A varying face background exposes repaint seams, including on Siamese shading.
    for y in 0..<size {
        ctx.setFillColor(CGColor(red:0.09+Double(y)/1600,green:0.07,blue:0.08,alpha:1))
        ctx.fill(CGRect(x:0,y:y,width:size,height:1))
    }
    if closure >= 0 {
        ctx.translateBy(x:64,y:64)
        ctx.scaleBy(x:2,y:2)
        if original { OriginalRenderer.drawEye3D(in:ctx,isLeft:left,yawRad:0,pitchRad:0,physics:physics,eyeTheme:theme,coat:coat) }
        else { FixedRenderer.drawEye3D(in:ctx,isLeft:left,yawRad:0,pitchRad:0,physics:physics,eyeTheme:theme,skin:SkinSpec.builtIn(coat)) }
    }
    return Array(UnsafeBufferPointer(start:ctx.data!.assumingMemoryBound(to:UInt8.self),count:size*size*4))
}
let baseline = render(false,.classicBlack,-1,true)
var oldFringe = 0
for coat in CatCoatTheme.allCases {
    for left in [true,false] {
        for closure: CGFloat in [0,0.25,0.5,0.92,1] {
            let new = render(false,coat,closure,left)
            let old = render(true,coat,closure,left)
            // Upper half is fully covered once closure >= 0.5; it must be untouched.
            if closure >= 0.5 {
                for y in 20..<57 {
                    for x in 0..<size {
                        // Bitmap storage runs from the top row; CoreGraphics user-space y runs upward.
                        let i=((size - 1 - y)*size+x)*4
                        precondition(Array(new[i..<i+4]) == Array(baseline[i..<i+4]),"Covered face changed: coat=\(coat), left=\(left), closure=\(closure), x=\(x), y=\(y)")
                        if Array(old[i..<i+4]) != Array(baseline[i..<i+4]) { oldFringe += 1 }
                    }
                }
            }
            if closure == 0 { precondition(new == old,"Open-eye appearance changed") }
            if closure == 1 { precondition(new == baseline,"Fully closed eye leaves pixels") }
        }
    }
}
for coat in CatCoatTheme.allCases {
    for theme in EyeColorTheme.allCases {
        for left in [true,false] {
            precondition(render(false,coat,1,left,theme) == baseline,"Closed iris leaves pixels: \(theme)")
            let partial = render(false,coat,0.92,left,theme)
            for y in 20..<57 {
                for x in 0..<size {
                    let i=((size-1-y)*size+x)*4
                    precondition(Array(partial[i..<i+4]) == Array(baseline[i..<i+4]),"Covered iris ring leaks")
                }
            }
        }
    }
}
precondition(oldFringe > 0,"Fixture did not reproduce original defect")
print("PASS: both eyes / four skins / all six eye themes; five legacy closure states; open eyes unchanged, covered pixels untouched, fully closed eyes invisible. Original contaminated pixels: \(oldFringe)")
