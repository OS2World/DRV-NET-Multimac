/* $Id: wat2map.cmd,v 1.2 2002/04/26 23:09:44 smilcke Exp $ */

/* SCCSID = src/dev/mme/pciaudio/wat2map.cmd, pciaudio, c.basedd 99/08/20 */
/****************************************************************************
 *                                                                          *
 * Copyright (c) IBM Corporation 1994 - 1997.                               *
 *                                                                          *
 * The following IBM OS/2 source code is provided to you solely for the     *
 * the purpose of assisting you in your development of OS/2 device drivers. *
 * You may use this code in accordance with the IBM License Agreement       *
 * provided in the IBM Device Driver Source Kit for OS/2.                   *
 *                                                                          *
 ****************************************************************************/

/* 05 Jul 05 SHL - Correct do/end errors, close files
   05 Jul 05 SHL - Partial 16/32 bit support (see f32bit)
*/

/**@internal src/dev/mme/pciaudio/wat2map.cmd, pciaudio, c.basedd
 * WAT2MAP - translate symbol map from Watcom format to MS format.
 * @version 1.1
 * @context
 *  Unless otherwise noted, all interfaces are Ring-0, 16-bit, kernel stack.
 * @notes
 *  Usage:  WAT2MAP <watcom_mapfile >ms_mapfile
 *               - or -
 *          type watcom_mapfile | WAT2MAP >ms_mapfile
 *
 *          Reads from stdin, writes to stdout.  Will accept the Watcom map filename
 *          as an argument (in place of reading from stdin).  Eg.,
 *
 *          WAT2MAP watcom_mapfile >ms_mapfile
 *
 *  Notes:
 *    1.)  The symbol handling in the debug kernel won't work for some of the
 *         characters used in the C++ name space.  WAT2MAP handles these symbols
 *         as follows.
 *             Scoping operator symbol '::' is translated to '__'.
 *             Destructor symbol '~' is translated to 'd'.
 *             Symbols for operators '::operator' are not provided.
 *
 *         Eg., for user defined class 'A', the symbol for constructor A::A is
 *         translated to A__A, and the destructor symbol A::~A becomes A__dA, and
 *         the assignment operator, 'A::operator =', is not translated.
 *
 *    2.)  Bug - C++ provides for defining multiple functions with same fn name
 *         but different function signatures (different parameter lists).  This
 *         utility just translates all the address / symbol combinations it finds,
 *         so you can end up with several addresses for a given fn name.
 * @history
*/
/* <End of helpText> - don't modify this string - used to flag end of help. */
/****************************************************************************/

CALL RXFUNCADD 'sysloadfuncs','rexxutil','sysloadfuncs'
call sysloadfuncs

f32bit=0;

Parse Arg arg1 arg2 rest
If (Length( arg1 ) = 0) | (Verify( arg1, '/-?' ) = 0) Then Do;
   Do i = 1 to 1000
      helpText = Sourceline(i)
      If Pos( '<End of helpText>', helpText ) <> 0 Then Leave;       /* quit loop */
      Say helpText
   End;
   Return
End;
If Length( arg2 ) = 0 Then Do;
   Say " Way to go Beaver... How about an out-put file name ?"
   Return
End;
mapFile = arg1          /* Can be Null, in which case we pull from stdin. */
outFile = arg2

/* erase outfile */  /* kill the old map file */
rc=SysFileDelete(outfile)


/*--- 1.  Find & translate module name.  ---*/
Do While Lines( mapFile ) <> 0
   watcomText = LineIn( mapFile )
   Parse Value watcomText With "Executable Image: " fileName
   If fileName <> "" Then Do;   /* Found match */
      fileName = FILESPEC('name', fileName);
      i = POS('.', fileName);
      if (i>0) then i=i-1;
      fileName = SUBSTR(fileName, 1, i);
      call lineout outfile ,' '
      call lineout outfile ,' ' || fileName;
      call lineout outfile ,' '
      Leave;                    /* Break from loop. */
   End;
End
If Lines( mapFile ) = 0 Then Do;        /* If end of file ... */
   Say "Error:  Expected to find line with text 'Executable Image:' "
   Return
End

/*--- 2.  Skip the group definitions - Rob's notes say we don't need them. -*/

/*--- 3.  Skip to the start of the segment table.  ---*/
Do While Lines( mapFile ) <> 0
   watcomText = LineIn( mapFile )
   Parse Value watcomText With "Segment" header_2_3 "Address" header_5
   If Strip( header_5 ) = "Size" Then Leave;      /* Found header line for Segment table. */
End
If Lines( mapFile ) = 0 Then Do;        /* If end of file ... */
   Say "Error:  Expected to find line with text 'Segments ... Size' "
   Return
End
junk = LineIn( mapFile )       /* Discard a couple lines of formatting. */
junk = LineIn( mapFile )

/*--- 4.  Translate segment table.  ---*/
NeedHeader=1;

Do While Lines( mapFile ) <> 0
   watcomText = LineIn( mapFile )
   Parse Value watcomText With segName className groupName address size .
   If segName = "" Then Leave;          /* Empty line, break from loop. */
   if (length(address)>9) then f32bit=1;
   if (NeedHeader) then do;
     if f32bit then call lineout outfile , " Start         Length     Name                   Class"
     else call lineout outfile , " Start     Length     Name                   Class"
     NeedHeader=0;
   end;
   if f32bit then
     length = size || 'H  '
   else
     length = Substr( size, 4, 5 ) || 'H     '
   segName = Left( segName, 23 )
   call lineout outfile ,' ' || address || ' ' || length || segName || className
End
call lineout outfile ,' '     /* Extra line feed. */


/*--- 5.  For all remaining lines in the input file:  if the line starts
   with a 16:16 address, assume it's a symbol declaration and translate
   it into MS format.  ---*/

call lineout outfile ,'  Address         Publics by Value'
/* call lineout outfile ,' '*/

LineTbl.0=0;
LineIX = 0;
Segments.0=0;

Do While Lines( mapFile ) <> 0
   watcomText = LineIn( mapFile )
   Parse Value watcomText With seg ':' ofs declaration
   if (seg=0) then iterate;
   if length(ofs) = 5 then
     ofs = left(ofs, 4) || strip(translate(right(ofs, 1),, '+*'))
   if length(ofs) = 9 then
     ofs = left(ofs, 8) || strip(translate(right(ofs, 1),, '+*'))

   is_Adress = (is_Hex(seg) = 1) & (is_Hex(ofs) = 1)
   If (is_Adress = 1) Then Do;

      SegIX = X2D(seg);
      if (SegIX>Segments.0) then do;
        do i=Segments.0 to SegIX;
           Segments.i=0;
        end;
        Segments.0=SegIX;
      end;
      if (X2D(ofs)>Segments.SegIX) then Segments.SegIX=X2D(ofs);

      if \ f32bit then
        ofs = right(ofs, 4)

      /*--- Haven't done the work to xlate operator symbols - skip the line. */
      If Pos( '::operator', declaration ) <> 0 Then Iterate;

      /*--- Strip any arguement list if this is a function prototype.  */
      declaration = StripMatchedParen( declaration )

      /*--- Strip array brackets if this is an array. */
      sqBracket = Pos( '[', declaration );
      If sqBracket <> 0
         Then declaration = Substr(declaration, 1, sqBracket-1);

      /*--- Strip leading tokens from the function name.
         Eg., remove function return type, near/far, etc.  */
      declaration = Word( declaration, Words(declaration) );

      /*--- Strip any remaining parens around function name. ---*/
      declaration = ReplaceSubstr( '(', ' ', declaration );
      declaration = ReplaceSubstr( ')', ' ', declaration );

      /*--- Debug kernel doesn't like symbol for scoping operator "::"
         in symbol names.  Replace :: with double underscore "__". ---*/
      declaration = ReplaceSubstr( '::', '__', declaration );

      /*--- Debug kernel doesn't like symbol for destructor "~"
         in symbol names.  Replace ~ with character "d" for "destructor.
         Note destructor for a class will translate "A::~A" -> "A__dA". ---*/
      declaration = ReplaceSubstr( '~', 'd', declaration );

      /* remove any trailing underscore */
      declaration = STRIP(declaration, 't', '_');

      /* remove the first underscore (if any) */
      if (LEFT(declaration,1) = '_') then do;
        declaration = RIGHT(declaration, LENGTH(declaration)-1);
      end;

      fill = copies(' ', 7)
      LineIX = LineIX + 1;
      LineTbl.LineIX = seg || ':' || ofs || fill || declaration;
      LineTbl.0 = LineIX;
   End;
End; /* End While through symbol section, end of input file. */

/* add dummy symbols for empty segments */
do i=1 to Segments.0;
  if (Segments.i>0) then iterate;
  LineIX = LineIX + 1;
  if (f32bit) then LineTbl.LineIX = D2X(i,4)||':00000000       SEGMENT'||i||'_DUMMY';
  else LineTbl.LineIX = D2X(i,4)||':0000       SEGMENT'||i||'_DUMMY';
  LineTbl.0 = LineIX;
end;

/* add dummy end segment symbols for 32 bit symbols */
if (f32bit) then do;
  do i=1 to Segments.0;
    if (Segments.i > X2D('10000')) then iterate;
    LineIX = LineIX + 1;
    LineTbl.LineIX = D2X(i,4)||':00010000       SEGMENT'||i||'_END';
    LineTbl.0 = LineIX;
  end;
end;

/* Sort the data */
do i=2 to LineTbl.0;
   t=LineTbl.i;
   do j=i-1 to 1 by -1 while t<LineTbl.j;
      k=j+1;
      LineTbl.k=LineTbl.j;
   end;
   j=j+1;
   LineTbl.j=t;
end;

/* output the data */
do LineIX = 1 to LineTbl.0;
  call lineout outfile , ' ' || LineTbl.LineIX;
end;

call stream mapfile, 'C', 'CLOSE'
call lineout outfile

Return;  /* End of program.  */

/*--- Helper subroutines. ---*/

StripMatchedParen:
/* Strips matched "( )" from end of string.  Returns
   a substring with the trailing, matched parens deleted. */

   Parse Arg string

   ixOpenParen = LastPos( "(", string );
   ixCloseParen = LastPos( ")", Substr( string, 1, Length(string)-1 ));

   If (ixOpenParen = 0)                     /* No match. */
      Then Return string
   Else If ixCloseParen < ixOpenParen       /* Found match, no imbedded "()". */
      Then Return Substr( string, 1, ixOpenParen-1 )
   Else Do;                                 /* Imbedded (), must skip over them. */
      /* Parse Value string With first ixCloseParen+1 rest */
      first = Substr( string, 1, ixCloseParen)
      rest = Substr( string, ixCloseParen+1 )
      string = StripMatchedParen( first ) || rest
      Return StripMatchedParen( string )
   End;


ReplaceSubstr:
/* Replaces oldPat (old pattern) with newPat (new pattern) in string. */

   Parse Arg oldPat , newPat , string

   ix = Pos( oldPat, string )
   if ix <> 0 Then Do;
      first = Substr( string, 1, ix-1 )
      rest  = Substr( string, ix + Length( oldPat ) )
      string = first || newPat || rest
   End;
   Return string


is_Hex:
/* Returns 1 if String is valid hex number, 0 otherwise. */
   Parse Arg string
   Return (Length(string) > 0) &  (Verify( string, '0123456789abcdefABCDEF' ) = 0)
