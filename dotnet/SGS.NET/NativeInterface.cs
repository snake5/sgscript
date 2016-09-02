﻿using System;
using System.Runtime.InteropServices;

namespace SGScript
{
	public class SGSException : Exception
	{
		public static string MakeExceptionMessage( int code, string additionalInfo )
		{
			string codeMsg = NI.ResultToString( code );
			if( additionalInfo != null && additionalInfo != "" )
				return string.Format( "{0} ({1})", additionalInfo, codeMsg );
			else
				return codeMsg;
		}

		public SGSException( int code, string additionalInfo = "" ) : base( MakeExceptionMessage( code, additionalInfo ) )
		{
			resultCode = code;
		}

		int resultCode;
	}

	// native interface
	public class NI
	{
		// message level presets
		public const int INFO    = 100;
		public const int WARNING = 200;
		public const int ERROR   = 300;
		public const int APIERR  = 330;
		public const int INTERR  = 360;

		// result codes
		public const int SUCCESS  = 0;
		public const int ENOTFND = -1;
		public const int ECOMP   = -2;
		public const int ENOTSUP = -4;
		public const int EBOUNDS = -5;
		public const int EINVAL  = -6;
		public const int EINPROC = -7;

		// variable types
		public const int VT_NULL   = 0;
		public const int VT_BOOL   = 1;
		public const int VT_INT    = 2;
		public const int VT_REAL   = 3;
		public const int VT_STRING = 4;
		public const int VT_FUNC   = 5;
		public const int VT_CFUNC  = 6;
		public const int VT_OBJECT = 7;
		public const int VT_PTR    = 8;
		public const int VT_THREAD = 9;
		public const int VT__COUNT = 10;

		// CodeString codes
		public const int CODE_ER = 0; /* error codes */
		public const int CODE_VT = 1; /* variable type */
		public const int CODE_OP = 2; /* VM instruction */

		// version info
		public const string Version = "1.4.0";
		public const int VersionMajor = 1;
		public const int VersionMinor = 4;
		public const int VersionIncr = 0;


		// helper functions
		public static string ResultToString( int result )
		{
			if( result >= 0 )
			{
				return string.Format( "[SUCCESS] Operation was successful (value={0})", result );
			}
			switch( result )
			{
				case ENOTFND: return "[ENOTFND] Item was not found";
				case ECOMP:   return "[ECOMP] Compiler error";
				case ENOTSUP: return "[ENOTSUP] Operation not supported";
				case EBOUNDS: return "[EBOUNDS] Index out of bounds";
				case EINVAL:  return "[EINVAL] Invalid value";
				case EINPROC: return "[EINPROC] Procedure failed";
				default: return string.Format( "Unknown result ({0})", result );
			}
		}
		public static int ResultToException( int result, string additionalInfo = "" )
		{
			if( result < 0 )
				throw new SGSException( result, additionalInfo );
			return result;
		}


		[StructLayout(LayoutKind.Explicit, Size=8)]
		public struct VarData
		{
			[FieldOffset(0)] public Int32 B;
			[FieldOffset(0)] public Int64 I;
			[FieldOffset(0)] public Double R;
			[FieldOffset(0)] public IntPtr S;
			[FieldOffset(0)] public IntPtr F;
			[FieldOffset(0)] public IntPtr C;
			[FieldOffset(0)] public IntPtr O;
			[FieldOffset(0)] public IntPtr P;
			[FieldOffset(0)] public IntPtr T;
		};
		public struct Variable
		{
			public UInt32  type;
			public VarData data;
		};


		public interface IUserData {}

		[UnmanagedFunctionPointer( CallingConvention.Cdecl )]
		public delegate IntPtr MemFunc( IUserData ud, IntPtr ptr, IntPtr size );

		[UnmanagedFunctionPointer( CallingConvention.Cdecl )]
		public delegate void PrintFunc( IUserData ud, IntPtr ctx, int type, [MarshalAs( UnmanagedType.LPStr )] string message );


		public static IntPtr DefaultMemFunc( IUserData ud, IntPtr ptr, IntPtr size )
		{
			if( ptr != IntPtr.Zero && size != IntPtr.Zero ) return Marshal.ReAllocHGlobal( ptr, size );
			else if( size != IntPtr.Zero ) return Marshal.AllocHGlobal( size );
			if( ptr != IntPtr.Zero ) Marshal.FreeHGlobal( ptr );
			return IntPtr.Zero;
		}


		[DllImport( "sgscript.dll", EntryPoint = "sgs_CreateEngineExt", CallingConvention = CallingConvention.Cdecl )]
		public static extern IntPtr CreateEngineExt( MemFunc mf, IUserData mfuserdata );

		public static IntPtr CreateEngine()
		{
			return CreateEngineExt( new MemFunc( DefaultMemFunc ), null );
		}

		[DllImport( "sgscript.dll", EntryPoint = "sgs_DestroyEngine", CallingConvention = CallingConvention.Cdecl )]
		public static extern void DestroyEngine( IntPtr ctx );

		[DllImport( "sgscript.dll", EntryPoint = "sgs_RootContext", CallingConvention = CallingConvention.Cdecl )]
		public static extern IntPtr RootContext( IntPtr ctx );
		
		[DllImport( "sgscript.dll", EntryPoint = "sgs_ForkState", CallingConvention = CallingConvention.Cdecl )]
		public static extern IntPtr ForkState( IntPtr ctx, int copystate );
		
		[DllImport( "sgscript.dll", EntryPoint = "sgs_ReleaseState", CallingConvention = CallingConvention.Cdecl )]
		public static extern void ReleaseState( IntPtr ctx );
		
		
		[DllImport( "sgscript.dll", EntryPoint = "sgs_CodeString", CallingConvention = CallingConvention.Cdecl )]
		[return: MarshalAs( UnmanagedType.LPStr )]
		public static extern string CodeString( int type, int val );


		[DllImport( "sgscript.dll", EntryPoint = "sgs_SetPrintFunc", CallingConvention = CallingConvention.Cdecl )]
		public static extern void SetPrintFunc( IntPtr ctx, PrintFunc pfn, IUserData printer );


		[DllImport( "sgscript.dll", EntryPoint = "sgs_EvalBuffer", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi )]
		public static extern int EvalBuffer( IntPtr ctx, [MarshalAs( UnmanagedType.LPStr )] string buf, Int32 bufsz );
		
		[DllImport( "sgscript.dll", EntryPoint = "sgs_EvalFile", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi )]
		public static extern int EvalFile( IntPtr ctx, [MarshalAs( UnmanagedType.LPStr )] string buf );
		
		[DllImport( "sgscript.dll", EntryPoint = "sgs_AdjustStack", CallingConvention = CallingConvention.Cdecl )]
		public static extern int AdjustStack( IntPtr ctx, int expected, int ret );
		
		[DllImport( "sgscript.dll", EntryPoint = "sgs_IncludeExt", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi )]
		public static extern int Include( IntPtr ctx, [MarshalAs( UnmanagedType.LPStr )] string name, [MarshalAs( UnmanagedType.LPStr )] string searchpath = null );
		
		public static int Eval( IntPtr ctx, string str ) { return EvalBuffer( ctx, str, str.Length ); }
		public static int Exec( IntPtr ctx, string str ) { return AdjustStack( ctx, 0, Eval( ctx, str ) ); }
		public static int ExecFile( IntPtr ctx, string str ) { return AdjustStack( ctx, 0, EvalFile( ctx, str ) ); }

		[DllImport( "sgscript.dll", EntryPoint = "sgs_PushNull", CallingConvention = CallingConvention.Cdecl )]
		public static extern void PushNull( IntPtr ctx );

		[DllImport( "sgscript.dll", EntryPoint = "sgs_PushBool", CallingConvention = CallingConvention.Cdecl )]
		public static extern void PushBool( IntPtr ctx, Int32 v );
		
		[DllImport( "sgscript.dll", EntryPoint = "sgs_PushInt", CallingConvention = CallingConvention.Cdecl )]
		public static extern void PushInt( IntPtr ctx, Int64 v );

		[DllImport( "sgscript.dll", EntryPoint = "sgs_PushStringBuf", CallingConvention = CallingConvention.Cdecl )]
		public static extern void PushStringBuf( IntPtr ctx, [MarshalAs( UnmanagedType.LPStr )] string buf, Int32 bufsz );

		[DllImport( "sgscript.dll", EntryPoint = "sgs_PushString", CallingConvention = CallingConvention.Cdecl )]
		public static extern void PushNullTerminatedString( IntPtr ctx, [MarshalAs( UnmanagedType.LPStr )] string str );
		

		[DllImport( "sgscript.dll", EntryPoint = "sgs_Pop", CallingConvention = CallingConvention.Cdecl )]
		public static extern void Pop( IntPtr ctx, Int32 count );
		
		[DllImport( "sgscript.dll", EntryPoint = "sgs_PopSkip", CallingConvention = CallingConvention.Cdecl )]
		public static extern void PopSkip( IntPtr ctx, Int32 count, Int32 skip );
		

		[DllImport( "sgscript.dll", EntryPoint = "sgs_StackSize", CallingConvention = CallingConvention.Cdecl )]
		public static extern Int32 StackSize( IntPtr ctx );

		[DllImport( "sgscript.dll", EntryPoint = "sgs_SetStackSize", CallingConvention = CallingConvention.Cdecl )]
		public static extern void SetStackSize( IntPtr ctx, Int32 size );

		[DllImport( "sgscript.dll", EntryPoint = "sgs_SetDeltaSize", CallingConvention = CallingConvention.Cdecl )]
		public static extern void SetDeltaSize( IntPtr ctx, Int32 diff );

		[DllImport( "sgscript.dll", EntryPoint = "sgs_AbsIndex", CallingConvention = CallingConvention.Cdecl )]
		public static extern Int32 AbsIndex( IntPtr ctx, Int32 item );

		[DllImport( "sgscript.dll", EntryPoint = "sgs_IsValidIndex", CallingConvention = CallingConvention.Cdecl )]
		public static extern int IsValidIndex( IntPtr ctx, Int32 item );

		[DllImport( "sgscript.dll", EntryPoint = "sgs_OptStackItem", CallingConvention = CallingConvention.Cdecl )]
		public static extern Variable OptStackItem( IntPtr ctx, Int32 item );

		[DllImport( "sgscript.dll", EntryPoint = "sgs_StackItem", CallingConvention = CallingConvention.Cdecl )]
		public static extern Variable StackItem( IntPtr ctx, Int32 item );

		// TODO GetStackItem

		[DllImport( "sgscript.dll", EntryPoint = "sgs_ItemType", CallingConvention = CallingConvention.Cdecl )]
		public static extern UInt32 ItemType( IntPtr ctx, Int32 item );
	}
}
