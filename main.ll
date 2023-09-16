; ModuleID = 'Mascal'
source_filename = "Mascal"

; Function Attrs: mustprogress nofree norecurse nosync nounwind readnone willreturn
define ghccc i32 @main() #0 {
entry:
  br i1 true, label %while, label %continue

while:                                            ; preds = %while, %entry
  %phi1 = phi i32 [ %1, %while ], [ 0, %entry ]
  %phi = phi i32 [ %0, %while ], [ 0, %entry ]
  %0 = add i32 %phi, 10
  %1 = add i32 %phi1, 1
  %cmptmp = icmp ugt i32 1000, %1
  br i1 %cmptmp, label %while, label %continue

continue:                                         ; preds = %while, %entry
  %phi2 = phi i32 [ %1, %while ], [ 0, %entry ]
  %phi3 = phi i32 [ %0, %while ], [ 0, %entry ]
  ret i32 %phi3
}

attributes #0 = { mustprogress nofree norecurse nosync nounwind readnone willreturn }
