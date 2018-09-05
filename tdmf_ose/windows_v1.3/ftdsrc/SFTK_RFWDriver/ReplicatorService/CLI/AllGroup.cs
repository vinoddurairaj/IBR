using System;

namespace CLI
{
	/// <summary>
	/// Summary description for AllGroup.
	/// </summary>
	public class AllGroup : Command
	{

		public bool	m_All = false;
		public bool	m_Group = false;


		public AllGroup()
		{
		}

		/// <summary>
		/// Displays help
		/// </summary>
		/// <param name="name"></param>
		public virtual void Usage( bool exit ) {
			Console.WriteLine( String.Format( "    -a              : {0} all groups", this.m_CommandName ));
			Console.WriteLine( String.Format( "    -g <group_num>  : {0} the group", this.m_CommandName ) );
		}

		/// <summary>
		/// process argument vector 
		/// </summary>
		/// <param name="args"></param>
		public virtual bool	Proc_Args( string [] args, int index ) {
			return Proc_Args( args, index, true );
		}

		/// <summary>
		/// process argument vector 
		/// </summary>
		/// <param name="args"></param>
		public virtual bool	Proc_Args( string [] args, int index, bool All ) {
			switch ( args[ index ].ToLower() ) {
				case "-a":
				case "/a":
					if ( All ) {
						// change the operation mode of all devices in all logical groups
						if ( m_Group ) {
							Usage( true );
						}
						m_All = true;
					} else {
						Usage( true );
					}
					break;
				case "-g":
				case "/g":
					// change the operation mode of all devices in a logical group
					if ( m_All ) {
						Usage( true );
					}
					if ( args.Length < ++index ) {
						Console.WriteLine( "Group option requires a valid group number" ); 
						Usage( true );
					}
					string number = args[ index ];
				getGroup:
					m_Group = true;
					if ( number == null ) {
						Console.WriteLine( "Group option requires a valid group number" ); 
						Usage( true );
					}
					try {
						m_lgnum = Convert.ToInt32( number );
					} catch ( Exception ) {
						Console.WriteLine( "Group option requires a valid group number" ); 
						Usage( true );
					}
					break;
				default:
					// -gxxx check
					if (( args[ index ].ToLower().StartsWith( "-g" )) ||
						( args[ index ].ToLower().StartsWith( "/g" ))) {
						number = args[ index ].Substring(2);
						goto getGroup;
					}
					return false;
			}
			return true;
		}


	}
}
