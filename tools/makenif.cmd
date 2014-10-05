/* rexx */
call RxFuncAdd 'SysLoadFuncs', 'RexxUtil', 'SysLoadFuncs';
call SysLoadFuncs;

parse arg InPath OutPath Version;

rc=SysFileDelete(OutPath);
Do While Lines( InPath ) <> 0;
  Text = LineIn( InPath );
  EqPos = POS("=", Text);
  If ( (POS("Version",Text)>0) & (EqPos>0)) then Do;   /* Found match */
    rc=LineOut(OutPath, SUBSTR(Text, 1, EqPos)||" "||Version);
  End;
  else rc=LineOut(OutPath, Text);
End;
rc=LineOut(OutPath);

