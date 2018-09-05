using System;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;
using System.Data;

namespace ServerFilterEditor
{
	/// <summary>
	/// Summary description for Form1.
	/// </summary>
	public class FormFilterEditor : System.Windows.Forms.Form
	{
		private ServerFilter m_ServerFilter = new ServerFilter();
		private bool m_bInitDone = false;
		private bool m_bModif = false;
		private System.Windows.Forms.Button ButtonOk;
		private System.Windows.Forms.Button buttonCancel;
		private System.Windows.Forms.CheckBox ServerSourceOnly;
		private System.Windows.Forms.Button buttonDeselectAllSource;
		private System.Windows.Forms.Button buttonSelectAllSource;
		private System.Windows.Forms.CheckedListBox checkedListBoxDeviceSource;
		private System.Windows.Forms.RadioButton radioButtonExcludeSource;
		private System.Windows.Forms.RadioButton radioButtonIncludeSource;
		private System.Windows.Forms.RadioButton radioButtonIncludeTarget;
		private System.Windows.Forms.Button buttonDeselectAllTarget;
		private System.Windows.Forms.RadioButton radioButtonExcludeTarget;
		private System.Windows.Forms.Button buttonSelectAllTarget;
		private System.Windows.Forms.CheckedListBox checkedListBoxDeviceTarget;
		private System.Windows.Forms.GroupBox groupBoxTarget;
		private System.Windows.Forms.GroupBox groupBoxSource;
		private System.Windows.Forms.GroupBox groupBoxServer;
		private System.Windows.Forms.Panel panelResize;
		private System.Windows.Forms.Splitter splitterResize;

		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.Container components = null;

		public FormFilterEditor()
		{
			//
			// Required for Windows Form Designer support
			//
			InitializeComponent();

			this.Icon = new System.Drawing.Icon(this.GetType(), "App.ico");

			if (m_ServerFilter.Load() == false)
			{
				MessageBox.Show("Error");
			}

			ServerSourceOnly.Checked = m_ServerFilter.SourceOnly;

			radioButtonIncludeSource.Checked = m_ServerFilter.IncludeSource;
			radioButtonExcludeSource.Checked = !m_ServerFilter.IncludeSource;
			foreach (DictionaryEntry DEDevice in m_ServerFilter.DevicesSource)
			{
				Device device = (Device)DEDevice.Value;
				checkedListBoxDeviceSource.Items.Add(device.Name, device.Selected);
			}

			radioButtonIncludeTarget.Checked = m_ServerFilter.IncludeTarget;
			radioButtonExcludeTarget.Checked = !m_ServerFilter.IncludeTarget;
			foreach (DictionaryEntry DEDevice in m_ServerFilter.DevicesTarget)
			{
				Device device = (Device)DEDevice.Value;
				checkedListBoxDeviceTarget.Items.Add(device.Name, device.Selected);
			}

			m_bInitDone = true;
		}

		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		protected override void Dispose( bool disposing )
		{
			if( disposing )
			{
				if (components != null) 
				{
					components.Dispose();
				}
			}
			base.Dispose( disposing );
		}

		#region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
			this.ButtonOk = new System.Windows.Forms.Button();
			this.buttonCancel = new System.Windows.Forms.Button();
			this.ServerSourceOnly = new System.Windows.Forms.CheckBox();
			this.groupBoxTarget = new System.Windows.Forms.GroupBox();
			this.radioButtonIncludeTarget = new System.Windows.Forms.RadioButton();
			this.buttonDeselectAllTarget = new System.Windows.Forms.Button();
			this.radioButtonExcludeTarget = new System.Windows.Forms.RadioButton();
			this.buttonSelectAllTarget = new System.Windows.Forms.Button();
			this.checkedListBoxDeviceTarget = new System.Windows.Forms.CheckedListBox();
			this.groupBoxSource = new System.Windows.Forms.GroupBox();
			this.radioButtonIncludeSource = new System.Windows.Forms.RadioButton();
			this.buttonDeselectAllSource = new System.Windows.Forms.Button();
			this.radioButtonExcludeSource = new System.Windows.Forms.RadioButton();
			this.buttonSelectAllSource = new System.Windows.Forms.Button();
			this.checkedListBoxDeviceSource = new System.Windows.Forms.CheckedListBox();
			this.groupBoxServer = new System.Windows.Forms.GroupBox();
			this.panelResize = new System.Windows.Forms.Panel();
			this.splitterResize = new System.Windows.Forms.Splitter();
			this.groupBoxTarget.SuspendLayout();
			this.groupBoxSource.SuspendLayout();
			this.groupBoxServer.SuspendLayout();
			this.panelResize.SuspendLayout();
			this.SuspendLayout();
			// 
			// ButtonOk
			// 
			this.ButtonOk.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.ButtonOk.DialogResult = System.Windows.Forms.DialogResult.OK;
			this.ButtonOk.Location = new System.Drawing.Point(208, 424);
			this.ButtonOk.Name = "ButtonOk";
			this.ButtonOk.Size = new System.Drawing.Size(80, 24);
			this.ButtonOk.TabIndex = 2;
			this.ButtonOk.Text = "OK";
			this.ButtonOk.Click += new System.EventHandler(this.ButtonOk_Click);
			// 
			// buttonCancel
			// 
			this.buttonCancel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.buttonCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
			this.buttonCancel.Location = new System.Drawing.Point(296, 424);
			this.buttonCancel.Name = "buttonCancel";
			this.buttonCancel.Size = new System.Drawing.Size(80, 24);
			this.buttonCancel.TabIndex = 3;
			this.buttonCancel.Text = "Cancel";
			this.buttonCancel.Click += new System.EventHandler(this.buttonCancel_Click);
			// 
			// ServerSourceOnly
			// 
			this.ServerSourceOnly.Location = new System.Drawing.Point(24, 24);
			this.ServerSourceOnly.Name = "ServerSourceOnly";
			this.ServerSourceOnly.Size = new System.Drawing.Size(240, 16);
			this.ServerSourceOnly.TabIndex = 0;
			this.ServerSourceOnly.Text = "Source Only";
			this.ServerSourceOnly.CheckedChanged += new System.EventHandler(this.ServerSourceOnly_CheckedChanged);
			// 
			// groupBoxTarget
			// 
			this.groupBoxTarget.Controls.Add(this.radioButtonIncludeTarget);
			this.groupBoxTarget.Controls.Add(this.buttonDeselectAllTarget);
			this.groupBoxTarget.Controls.Add(this.radioButtonExcludeTarget);
			this.groupBoxTarget.Controls.Add(this.buttonSelectAllTarget);
			this.groupBoxTarget.Controls.Add(this.checkedListBoxDeviceTarget);
			this.groupBoxTarget.Dock = System.Windows.Forms.DockStyle.Fill;
			this.groupBoxTarget.Location = new System.Drawing.Point(0, 170);
			this.groupBoxTarget.Name = "groupBoxTarget";
			this.groupBoxTarget.Size = new System.Drawing.Size(369, 174);
			this.groupBoxTarget.TabIndex = 11;
			this.groupBoxTarget.TabStop = false;
			this.groupBoxTarget.Text = "Target Devices";
			// 
			// radioButtonIncludeTarget
			// 
			this.radioButtonIncludeTarget.Checked = true;
			this.radioButtonIncludeTarget.Location = new System.Drawing.Point(8, 16);
			this.radioButtonIncludeTarget.Name = "radioButtonIncludeTarget";
			this.radioButtonIncludeTarget.Size = new System.Drawing.Size(72, 24);
			this.radioButtonIncludeTarget.TabIndex = 5;
			this.radioButtonIncludeTarget.TabStop = true;
			this.radioButtonIncludeTarget.Text = "Include";
			this.radioButtonIncludeTarget.CheckedChanged += new System.EventHandler(this.radioButtonIncludeTarget_CheckedChanged);
			// 
			// buttonDeselectAllTarget
			// 
			this.buttonDeselectAllTarget.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
			this.buttonDeselectAllTarget.Location = new System.Drawing.Point(281, 80);
			this.buttonDeselectAllTarget.Name = "buttonDeselectAllTarget";
			this.buttonDeselectAllTarget.Size = new System.Drawing.Size(80, 24);
			this.buttonDeselectAllTarget.TabIndex = 9;
			this.buttonDeselectAllTarget.Text = "Deselect All";
			this.buttonDeselectAllTarget.Click += new System.EventHandler(this.buttonDeselectAllTarget_Click);
			// 
			// radioButtonExcludeTarget
			// 
			this.radioButtonExcludeTarget.Location = new System.Drawing.Point(96, 16);
			this.radioButtonExcludeTarget.Name = "radioButtonExcludeTarget";
			this.radioButtonExcludeTarget.Size = new System.Drawing.Size(72, 24);
			this.radioButtonExcludeTarget.TabIndex = 6;
			this.radioButtonExcludeTarget.Text = "Exclude";
			// 
			// buttonSelectAllTarget
			// 
			this.buttonSelectAllTarget.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
			this.buttonSelectAllTarget.Location = new System.Drawing.Point(281, 48);
			this.buttonSelectAllTarget.Name = "buttonSelectAllTarget";
			this.buttonSelectAllTarget.Size = new System.Drawing.Size(80, 24);
			this.buttonSelectAllTarget.TabIndex = 8;
			this.buttonSelectAllTarget.Text = "Select All";
			this.buttonSelectAllTarget.Click += new System.EventHandler(this.buttonSelectAllTarget_Click);
			// 
			// checkedListBoxDeviceTarget
			// 
			this.checkedListBoxDeviceTarget.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
				| System.Windows.Forms.AnchorStyles.Left) 
				| System.Windows.Forms.AnchorStyles.Right)));
			this.checkedListBoxDeviceTarget.Location = new System.Drawing.Point(8, 48);
			this.checkedListBoxDeviceTarget.Name = "checkedListBoxDeviceTarget";
			this.checkedListBoxDeviceTarget.Size = new System.Drawing.Size(266, 109);
			this.checkedListBoxDeviceTarget.TabIndex = 7;
			this.checkedListBoxDeviceTarget.ItemCheck += new System.Windows.Forms.ItemCheckEventHandler(this.checkedListBoxDeviceTarget_ItemCheck);
			// 
			// groupBoxSource
			// 
			this.groupBoxSource.Controls.Add(this.radioButtonIncludeSource);
			this.groupBoxSource.Controls.Add(this.buttonDeselectAllSource);
			this.groupBoxSource.Controls.Add(this.radioButtonExcludeSource);
			this.groupBoxSource.Controls.Add(this.buttonSelectAllSource);
			this.groupBoxSource.Controls.Add(this.checkedListBoxDeviceSource);
			this.groupBoxSource.Dock = System.Windows.Forms.DockStyle.Top;
			this.groupBoxSource.Location = new System.Drawing.Point(0, 0);
			this.groupBoxSource.Name = "groupBoxSource";
			this.groupBoxSource.Size = new System.Drawing.Size(369, 167);
			this.groupBoxSource.TabIndex = 10;
			this.groupBoxSource.TabStop = false;
			this.groupBoxSource.Text = "Source Devices";
			// 
			// radioButtonIncludeSource
			// 
			this.radioButtonIncludeSource.Checked = true;
			this.radioButtonIncludeSource.Location = new System.Drawing.Point(8, 16);
			this.radioButtonIncludeSource.Name = "radioButtonIncludeSource";
			this.radioButtonIncludeSource.Size = new System.Drawing.Size(72, 24);
			this.radioButtonIncludeSource.TabIndex = 5;
			this.radioButtonIncludeSource.TabStop = true;
			this.radioButtonIncludeSource.Text = "Include";
			this.radioButtonIncludeSource.CheckedChanged += new System.EventHandler(this.radioButtonIncludeSource_CheckedChanged);
			// 
			// buttonDeselectAllSource
			// 
			this.buttonDeselectAllSource.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
			this.buttonDeselectAllSource.Location = new System.Drawing.Point(281, 80);
			this.buttonDeselectAllSource.Name = "buttonDeselectAllSource";
			this.buttonDeselectAllSource.Size = new System.Drawing.Size(80, 24);
			this.buttonDeselectAllSource.TabIndex = 9;
			this.buttonDeselectAllSource.Text = "Deselect All";
			this.buttonDeselectAllSource.Click += new System.EventHandler(this.buttonDeselectAllSource_Click);
			// 
			// radioButtonExcludeSource
			// 
			this.radioButtonExcludeSource.Location = new System.Drawing.Point(96, 16);
			this.radioButtonExcludeSource.Name = "radioButtonExcludeSource";
			this.radioButtonExcludeSource.Size = new System.Drawing.Size(72, 24);
			this.radioButtonExcludeSource.TabIndex = 6;
			this.radioButtonExcludeSource.Text = "Exclude";
			// 
			// buttonSelectAllSource
			// 
			this.buttonSelectAllSource.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
			this.buttonSelectAllSource.Location = new System.Drawing.Point(281, 48);
			this.buttonSelectAllSource.Name = "buttonSelectAllSource";
			this.buttonSelectAllSource.Size = new System.Drawing.Size(80, 24);
			this.buttonSelectAllSource.TabIndex = 8;
			this.buttonSelectAllSource.Text = "Select All";
			this.buttonSelectAllSource.Click += new System.EventHandler(this.buttonSelectAllSource_Click);
			// 
			// checkedListBoxDeviceSource
			// 
			this.checkedListBoxDeviceSource.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
				| System.Windows.Forms.AnchorStyles.Left) 
				| System.Windows.Forms.AnchorStyles.Right)));
			this.checkedListBoxDeviceSource.Location = new System.Drawing.Point(8, 48);
			this.checkedListBoxDeviceSource.Name = "checkedListBoxDeviceSource";
			this.checkedListBoxDeviceSource.Size = new System.Drawing.Size(266, 109);
			this.checkedListBoxDeviceSource.TabIndex = 7;
			this.checkedListBoxDeviceSource.ItemCheck += new System.Windows.Forms.ItemCheckEventHandler(this.checkedListBoxDeviceSource_ItemCheck);
			// 
			// groupBoxServer
			// 
			this.groupBoxServer.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
				| System.Windows.Forms.AnchorStyles.Right)));
			this.groupBoxServer.Controls.Add(this.ServerSourceOnly);
			this.groupBoxServer.Location = new System.Drawing.Point(8, 8);
			this.groupBoxServer.Name = "groupBoxServer";
			this.groupBoxServer.Size = new System.Drawing.Size(368, 56);
			this.groupBoxServer.TabIndex = 12;
			this.groupBoxServer.TabStop = false;
			this.groupBoxServer.Text = "Server";
			// 
			// panelResize
			// 
			this.panelResize.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
				| System.Windows.Forms.AnchorStyles.Left) 
				| System.Windows.Forms.AnchorStyles.Right)));
			this.panelResize.Controls.Add(this.groupBoxTarget);
			this.panelResize.Controls.Add(this.splitterResize);
			this.panelResize.Controls.Add(this.groupBoxSource);
			this.panelResize.Location = new System.Drawing.Point(8, 72);
			this.panelResize.Name = "panelResize";
			this.panelResize.Size = new System.Drawing.Size(369, 344);
			this.panelResize.TabIndex = 13;
			// 
			// splitterResize
			// 
			this.splitterResize.Dock = System.Windows.Forms.DockStyle.Top;
			this.splitterResize.Location = new System.Drawing.Point(0, 167);
			this.splitterResize.MinExtra = 110;
			this.splitterResize.MinSize = 110;
			this.splitterResize.Name = "splitterResize";
			this.splitterResize.Size = new System.Drawing.Size(369, 3);
			this.splitterResize.TabIndex = 10;
			this.splitterResize.TabStop = false;
			// 
			// FormFilterEditor
			// 
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.ClientSize = new System.Drawing.Size(384, 454);
			this.Controls.Add(this.buttonCancel);
			this.Controls.Add(this.ButtonOk);
			this.Controls.Add(this.panelResize);
			this.Controls.Add(this.groupBoxServer);
			this.MinimumSize = new System.Drawing.Size(392, 488);
			this.Name = "FormFilterEditor";
			this.Text = "Server Devices Filter Editor";
			this.groupBoxTarget.ResumeLayout(false);
			this.groupBoxSource.ResumeLayout(false);
			this.groupBoxServer.ResumeLayout(false);
			this.panelResize.ResumeLayout(false);
			this.ResumeLayout(false);

		}
		#endregion

		/// <summary>
		/// The main entry point for the application.
		/// </summary>
		[STAThread]
		static void Main() 
		{
			Application.Run(new FormFilterEditor());
		}

		private void ServerSourceOnly_CheckedChanged(object sender, System.EventArgs e)
		{
			groupBoxTarget.Enabled             = !ServerSourceOnly.Checked;
			radioButtonIncludeTarget.Enabled   = !ServerSourceOnly.Checked;
			radioButtonExcludeTarget.Enabled   = !ServerSourceOnly.Checked;
			checkedListBoxDeviceTarget.Enabled = !ServerSourceOnly.Checked;
			buttonSelectAllTarget.Enabled      = !ServerSourceOnly.Checked;
			buttonDeselectAllTarget.Enabled    = !ServerSourceOnly.Checked;

			// New change
			if (m_bInitDone)
			{
				m_ServerFilter.SourceOnly = ServerSourceOnly.Checked;
				m_bModif = true;
			}
		}

		private void radioButtonIncludeSource_CheckedChanged(object sender, System.EventArgs e)
		{
			// New change
			if (m_bInitDone)
			{
				m_ServerFilter.IncludeSource = radioButtonIncludeSource.Checked;
				m_bModif = true;
			}
		}

		private void checkedListBoxDeviceSource_ItemCheck(object sender, System.Windows.Forms.ItemCheckEventArgs e)
		{
			// New change
			if (m_bInitDone)
			{
				((Device)m_ServerFilter.DevicesSource.GetByIndex(e.Index)).Selected = (e.NewValue == CheckState.Checked);
				m_bModif = true;
			}
		}

		private void buttonSelectAllSource_Click(object sender, System.EventArgs e)
		{
			for (int nIndex = 0; nIndex < checkedListBoxDeviceSource.Items.Count; nIndex++)
			{
				checkedListBoxDeviceSource.SetItemChecked(nIndex, true);
			}
		}

		private void buttonDeselectAllSource_Click(object sender, System.EventArgs e)
		{
			for (int nIndex = 0; nIndex < checkedListBoxDeviceSource.Items.Count; nIndex++)
			{
				checkedListBoxDeviceSource.SetItemChecked(nIndex, false);
			}
		}

		private void radioButtonIncludeTarget_CheckedChanged(object sender, System.EventArgs e)
		{
			// New change
			if (m_bInitDone)
			{
				m_ServerFilter.IncludeTarget = radioButtonIncludeTarget.Checked;
				m_bModif = true;
			}		
		}

		private void checkedListBoxDeviceTarget_ItemCheck(object sender, System.Windows.Forms.ItemCheckEventArgs e)
		{
			// New change
			if (m_bInitDone)
			{
				((Device)m_ServerFilter.DevicesTarget.GetByIndex(e.Index)).Selected = (e.NewValue == CheckState.Checked);
				m_bModif = true;
			}		
		}

		private void buttonSelectAllTarget_Click(object sender, System.EventArgs e)
		{
			for (int nIndex = 0; nIndex < checkedListBoxDeviceTarget.Items.Count; nIndex++)
			{
				checkedListBoxDeviceTarget.SetItemChecked(nIndex, true);
			}		
		}

		private void buttonDeselectAllTarget_Click(object sender, System.EventArgs e)
		{
			for (int nIndex = 0; nIndex < checkedListBoxDeviceTarget.Items.Count; nIndex++)
			{
				checkedListBoxDeviceTarget.SetItemChecked(nIndex, false);
			}		
		}

		private void ButtonOk_Click(object sender, System.EventArgs e)
		{
			if (m_bModif)
			{
				// Save new info
				m_ServerFilter.Save();
				m_bModif = false;
			}

			Close();		
		}

		private void buttonCancel_Click(object sender, System.EventArgs e)
		{
			Close();
		}

		protected override void OnClosing(CancelEventArgs e)
		{
			// Determine if text has changed in the textbox by comparing to original text.
			if (m_bModif)
			{
				if (MessageBox.Show(this, "Are you sure you want to cancel all your changes?", "Warning",
					MessageBoxButtons.YesNo, MessageBoxIcon.Question, MessageBoxDefaultButton.Button1, 
					MessageBoxOptions.RightAlign) == DialogResult.No)
				{
					// Cancel the Closing event from closing the form.
					e.Cancel = true;
				}
			}

			base.OnClosing (e);
		}
	}
}
