#![allow(non_snake_case, unused, dead_code)]
/// This code is automatically @generated. Do not edit.
use crate::rw::{TableRow, escape, unescape};

#[derive(Debug, Default, Clone)]
pub struct ActionText {
    /// Name of action to be described.
    pub Action: String,
    /// Localized description displayed in progress dialog and log when action is executing.
    pub Description: String,
    /// Optional localized format template used to format action data records for display during action execution.
    pub Template: String,
}

impl ActionText {
    #[rustfmt::skip]
    pub fn new(Action: impl Into<String>, Description: impl Into<String>, Template: impl Into<String>) -> Self {
        Self {
            Action: Action.into(),
            Description: Description.into(),
            Template: Template.into(),
        }
    }
}

impl TableRow for ActionText {
    const NAME: &'static str = "ActionText";
    const COLUMNS: &'static str = "Action\tDescription\tTemplate";
    const TYPES: &'static str = "s72\tL0\tL0";
    const META: &'static str = "ActionText\tAction";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.Action.clone()))?;
        write!(writer, "{}\t", escape(self.Description.clone()))?;
        write!(writer, "{}\r\n", escape(self.Template.clone()))?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            Action: unescape(parts[0].to_string()),
            Description: unescape(parts[1].to_string()),
            Template: unescape(parts[2].to_string()),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct AdminExecuteSequence {
    /// Name of action to invoke, either in the engine or the handler DLL.
    pub Action: String,
    /// Optional expression which skips the action if evaluates to expFalse.If the expression syntax is invalid, the engine will terminate, returning iesBadActionData.
    pub Condition: String,
    /// Number that determines the sort order in which the actions are to be executed.  Leave blank to suppress action.
    pub Sequence: i16,
}

impl AdminExecuteSequence {
    #[rustfmt::skip]
    pub fn new(Action: impl Into<String>, Condition: impl Into<String>, Sequence: impl Into<i16>) -> Self {
        Self {
            Action: Action.into(),
            Condition: Condition.into(),
            Sequence: Sequence.into(),
        }
    }
}

impl TableRow for AdminExecuteSequence {
    const NAME: &'static str = "AdminExecuteSequence";
    const COLUMNS: &'static str = "Action\tCondition\tSequence";
    const TYPES: &'static str = "s72\tS255\tI2";
    const META: &'static str = "AdminExecuteSequence\tAction";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.Action.clone()))?;
        write!(writer, "{}\t", escape(self.Condition.clone()))?;
        write!(writer, "{}\r\n", self.Sequence)?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            Action: unescape(parts[0].to_string()),
            Condition: unescape(parts[1].to_string()),
            Sequence: parts[2].parse().unwrap(),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct AdminUISequence {
    /// Name of action to invoke, either in the engine or the handler DLL.
    pub Action: String,
    /// Optional expression which skips the action if evaluates to expFalse.If the expression syntax is invalid, the engine will terminate, returning iesBadActionData.
    pub Condition: String,
    /// Number that determines the sort order in which the actions are to be executed.  Leave blank to suppress action.
    pub Sequence: i16,
}

impl AdminUISequence {
    #[rustfmt::skip]
    pub fn new(Action: impl Into<String>, Condition: impl Into<String>, Sequence: impl Into<i16>) -> Self {
        Self {
            Action: Action.into(),
            Condition: Condition.into(),
            Sequence: Sequence.into(),
        }
    }
}

impl TableRow for AdminUISequence {
    const NAME: &'static str = "AdminUISequence";
    const COLUMNS: &'static str = "Action\tCondition\tSequence";
    const TYPES: &'static str = "s72\tS255\tI2";
    const META: &'static str = "AdminUISequence\tAction";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.Action.clone()))?;
        write!(writer, "{}\t", escape(self.Condition.clone()))?;
        write!(writer, "{}\r\n", self.Sequence)?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            Action: unescape(parts[0].to_string()),
            Condition: unescape(parts[1].to_string()),
            Sequence: parts[2].parse().unwrap(),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct AdvtExecuteSequence {
    /// Name of action to invoke, either in the engine or the handler DLL.
    pub Action: String,
    /// Optional expression which skips the action if evaluates to expFalse.If the expression syntax is invalid, the engine will terminate, returning iesBadActionData.
    pub Condition: String,
    /// Number that determines the sort order in which the actions are to be executed.  Leave blank to suppress action.
    pub Sequence: i16,
}

impl AdvtExecuteSequence {
    #[rustfmt::skip]
    pub fn new(Action: impl Into<String>, Condition: impl Into<String>, Sequence: impl Into<i16>) -> Self {
        Self {
            Action: Action.into(),
            Condition: Condition.into(),
            Sequence: Sequence.into(),
        }
    }
}

impl TableRow for AdvtExecuteSequence {
    const NAME: &'static str = "AdvtExecuteSequence";
    const COLUMNS: &'static str = "Action\tCondition\tSequence";
    const TYPES: &'static str = "s72\tS255\tI2";
    const META: &'static str = "AdvtExecuteSequence\tAction";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.Action.clone()))?;
        write!(writer, "{}\t", escape(self.Condition.clone()))?;
        write!(writer, "{}\r\n", self.Sequence)?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            Action: unescape(parts[0].to_string()),
            Condition: unescape(parts[1].to_string()),
            Sequence: parts[2].parse().unwrap(),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct AdvtUISequence {
    /// Name of action to invoke, either in the engine or the handler DLL.
    pub Action: String,
    /// Optional expression which skips the action if evaluates to expFalse.If the expression syntax is invalid, the engine will terminate, returning iesBadActionData.
    pub Condition: String,
    /// Number that determines the sort order in which the actions are to be executed.  Leave blank to suppress action.
    pub Sequence: i16,
}

impl AdvtUISequence {
    #[rustfmt::skip]
    pub fn new(Action: impl Into<String>, Condition: impl Into<String>, Sequence: impl Into<i16>) -> Self {
        Self {
            Action: Action.into(),
            Condition: Condition.into(),
            Sequence: Sequence.into(),
        }
    }
}

impl TableRow for AdvtUISequence {
    const NAME: &'static str = "AdvtUISequence";
    const COLUMNS: &'static str = "Action\tCondition\tSequence";
    const TYPES: &'static str = "s72\tS255\tI2";
    const META: &'static str = "AdvtUISequence\tAction";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.Action.clone()))?;
        write!(writer, "{}\t", escape(self.Condition.clone()))?;
        write!(writer, "{}\r\n", self.Sequence)?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            Action: unescape(parts[0].to_string()),
            Condition: unescape(parts[1].to_string()),
            Sequence: parts[2].parse().unwrap(),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct AppId {
    pub AppId: String,
    pub RemoteServerName: String,
    pub LocalService: String,
    pub ServiceParameters: String,
    pub DllSurrogate: String,
    pub ActivateAtStorage: i16,
    pub RunAsInteractiveUser: i16,
}

impl AppId {
    #[rustfmt::skip]
    pub fn new(AppId: impl Into<String>, RemoteServerName: impl Into<String>, LocalService: impl Into<String>, ServiceParameters: impl Into<String>, DllSurrogate: impl Into<String>, ActivateAtStorage: impl Into<i16>, RunAsInteractiveUser: impl Into<i16>) -> Self {
        Self {
            AppId: AppId.into(),
            RemoteServerName: RemoteServerName.into(),
            LocalService: LocalService.into(),
            ServiceParameters: ServiceParameters.into(),
            DllSurrogate: DllSurrogate.into(),
            ActivateAtStorage: ActivateAtStorage.into(),
            RunAsInteractiveUser: RunAsInteractiveUser.into(),
        }
    }
}

impl TableRow for AppId {
    const NAME: &'static str = "AppId";
    const COLUMNS: &'static str = "AppId\tRemoteServerName\tLocalService\tServiceParameters\tDllSurrogate\tActivateAtStorage\tRunAsInteractiveUser";
    const TYPES: &'static str = "s38\tS255\tS255\tS255\tS255\tI2\tI2";
    const META: &'static str = "AppId\tAppId";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.AppId.clone()))?;
        write!(writer, "{}\t", escape(self.RemoteServerName.clone()))?;
        write!(writer, "{}\t", escape(self.LocalService.clone()))?;
        write!(writer, "{}\t", escape(self.ServiceParameters.clone()))?;
        write!(writer, "{}\t", escape(self.DllSurrogate.clone()))?;
        write!(writer, "{}\t", self.ActivateAtStorage)?;
        write!(writer, "{}\r\n", self.RunAsInteractiveUser)?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            AppId: unescape(parts[0].to_string()),
            RemoteServerName: unescape(parts[1].to_string()),
            LocalService: unescape(parts[2].to_string()),
            ServiceParameters: unescape(parts[3].to_string()),
            DllSurrogate: unescape(parts[4].to_string()),
            ActivateAtStorage: parts[5].parse().unwrap(),
            RunAsInteractiveUser: parts[6].parse().unwrap(),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct AppSearch {
    /// The property associated with a Signature
    pub Property: String,
    /// The Signature_ represents a unique file signature and is also the foreign key in the Signature,  RegLocator, IniLocator, CompLocator and the DrLocator tables.
    pub Signature_: String,
}

impl AppSearch {
    #[rustfmt::skip]
    pub fn new(Property: impl Into<String>, Signature_: impl Into<String>) -> Self {
        Self {
            Property: Property.into(),
            Signature_: Signature_.into(),
        }
    }
}

impl TableRow for AppSearch {
    const NAME: &'static str = "AppSearch";
    const COLUMNS: &'static str = "Property\tSignature_";
    const TYPES: &'static str = "s72\ts72";
    const META: &'static str = "AppSearch\tProperty\tSignature_";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.Property.clone()))?;
        write!(writer, "{}\r\n", escape(self.Signature_.clone()))?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            Property: unescape(parts[0].to_string()),
            Signature_: unescape(parts[1].to_string()),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct BBControl {
    /// External key to the Billboard table, name of the billboard.
    pub Billboard_: String,
    /// Name of the control. This name must be unique within a billboard, but can repeat on different billboard.
    pub BBControl: String,
    /// The type of the control.
    pub Type: String,
    /// Horizontal coordinate of the upper left corner of the bounding rectangle of the control.
    pub X: i16,
    /// Vertical coordinate of the upper left corner of the bounding rectangle of the control.
    pub Y: i16,
    /// Width of the bounding rectangle of the control.
    pub Width: i16,
    /// Height of the bounding rectangle of the control.
    pub Height: i16,
    /// A 32-bit word that specifies the attribute flags to be applied to this control.
    pub Attributes: i32,
    /// A string used to set the initial text contained within a control (if appropriate).
    pub Text: String,
}

impl BBControl {
    #[rustfmt::skip]
    pub fn new(Billboard_: impl Into<String>, BBControl: impl Into<String>, Type: impl Into<String>, X: impl Into<i16>, Y: impl Into<i16>, Width: impl Into<i16>, Height: impl Into<i16>, Attributes: impl Into<i32>, Text: impl Into<String>) -> Self {
        Self {
            Billboard_: Billboard_.into(),
            BBControl: BBControl.into(),
            Type: Type.into(),
            X: X.into(),
            Y: Y.into(),
            Width: Width.into(),
            Height: Height.into(),
            Attributes: Attributes.into(),
            Text: Text.into(),
        }
    }
}

impl TableRow for BBControl {
    const NAME: &'static str = "BBControl";
    const COLUMNS: &'static str = "Billboard_\tBBControl\tType\tX\tY\tWidth\tHeight\tAttributes\tText";
    const TYPES: &'static str = "s50\ts50\ts50\ti2\ti2\ti2\ti2\tI4\tL50";
    const META: &'static str = "BBControl\tBillboard_\tBBControl";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.Billboard_.clone()))?;
        write!(writer, "{}\t", escape(self.BBControl.clone()))?;
        write!(writer, "{}\t", escape(self.Type.clone()))?;
        write!(writer, "{}\t", self.X)?;
        write!(writer, "{}\t", self.Y)?;
        write!(writer, "{}\t", self.Width)?;
        write!(writer, "{}\t", self.Height)?;
        write!(writer, "{}\t", self.Attributes)?;
        write!(writer, "{}\r\n", escape(self.Text.clone()))?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            Billboard_: unescape(parts[0].to_string()),
            BBControl: unescape(parts[1].to_string()),
            Type: unescape(parts[2].to_string()),
            X: parts[3].parse().unwrap(),
            Y: parts[4].parse().unwrap(),
            Width: parts[5].parse().unwrap(),
            Height: parts[6].parse().unwrap(),
            Attributes: parts[7].parse().unwrap(),
            Text: unescape(parts[8].to_string()),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct Billboard {
    /// Name of the billboard.
    pub Billboard: String,
    /// An external key to the Feature Table. The billboard is shown only if this feature is being installed.
    pub Feature_: String,
    /// The name of an action. The billboard is displayed during the progress messages received from this action.
    pub Action: String,
    /// A positive integer. If there is more than one billboard corresponding to an action they will be shown in the order defined by this column.
    pub Ordering: i16,
}

impl Billboard {
    #[rustfmt::skip]
    pub fn new(Billboard: impl Into<String>, Feature_: impl Into<String>, Action: impl Into<String>, Ordering: impl Into<i16>) -> Self {
        Self {
            Billboard: Billboard.into(),
            Feature_: Feature_.into(),
            Action: Action.into(),
            Ordering: Ordering.into(),
        }
    }
}

impl TableRow for Billboard {
    const NAME: &'static str = "Billboard";
    const COLUMNS: &'static str = "Billboard\tFeature_\tAction\tOrdering";
    const TYPES: &'static str = "s50\ts38\tS50\tI2";
    const META: &'static str = "Billboard\tBillboard";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.Billboard.clone()))?;
        write!(writer, "{}\t", escape(self.Feature_.clone()))?;
        write!(writer, "{}\t", escape(self.Action.clone()))?;
        write!(writer, "{}\r\n", self.Ordering)?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            Billboard: unescape(parts[0].to_string()),
            Feature_: unescape(parts[1].to_string()),
            Action: unescape(parts[2].to_string()),
            Ordering: parts[3].parse().unwrap(),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct BindImage {
    /// The index into the File table. This must be an executable file.
    pub File_: String,
    /// A list of ;  delimited paths that represent the paths to be searched for the import DLLS. The list is usually a list of properties each enclosed within square brackets [] .
    pub Path: String,
}

impl BindImage {
    #[rustfmt::skip]
    pub fn new(File_: impl Into<String>, Path: impl Into<String>) -> Self {
        Self {
            File_: File_.into(),
            Path: Path.into(),
        }
    }
}

impl TableRow for BindImage {
    const NAME: &'static str = "BindImage";
    const COLUMNS: &'static str = "File_\tPath";
    const TYPES: &'static str = "s72\tS255";
    const META: &'static str = "BindImage\tFile_";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.File_.clone()))?;
        write!(writer, "{}\r\n", escape(self.Path.clone()))?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            File_: unescape(parts[0].to_string()),
            Path: unescape(parts[1].to_string()),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct CCPSearch {
    /// The Signature_ represents a unique file signature and is also the foreign key in the Signature,  RegLocator, IniLocator, CompLocator and the DrLocator tables.
    pub Signature_: String,
}

impl CCPSearch {
    #[rustfmt::skip]
    pub fn new(Signature_: impl Into<String>) -> Self {
        Self {
            Signature_: Signature_.into(),
        }
    }
}

impl TableRow for CCPSearch {
    const NAME: &'static str = "CCPSearch";
    const COLUMNS: &'static str = "Signature_";
    const TYPES: &'static str = "s72";
    const META: &'static str = "CCPSearch\tSignature_";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\r\n", escape(self.Signature_.clone()))?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            Signature_: unescape(parts[0].to_string()),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct CheckBox {
    /// A named property to be tied to the item.
    pub Property: String,
    /// The value string associated with the item.
    pub Value: String,
}

impl CheckBox {
    #[rustfmt::skip]
    pub fn new(Property: impl Into<String>, Value: impl Into<String>) -> Self {
        Self {
            Property: Property.into(),
            Value: Value.into(),
        }
    }
}

impl TableRow for CheckBox {
    const NAME: &'static str = "CheckBox";
    const COLUMNS: &'static str = "Property\tValue";
    const TYPES: &'static str = "s72\tS64";
    const META: &'static str = "CheckBox\tProperty";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.Property.clone()))?;
        write!(writer, "{}\r\n", escape(self.Value.clone()))?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            Property: unescape(parts[0].to_string()),
            Value: unescape(parts[1].to_string()),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct Class {
    /// The CLSID of an OLE factory.
    pub CLSID: String,
    /// The numeric server context for this server. CLSCTX_xxxx
    pub Context: String,
    /// Required foreign key into the Component Table, specifying the component for which to return a path when called through LocateComponent.
    pub Component_: String,
    /// Optional ProgId associated with this CLSID.
    pub ProgId_Default: String,
    /// Localized description for the Class.
    pub Description: String,
    /// Optional AppID containing DCOM information for associated application (string GUID).
    pub AppId_: String,
    /// Optional string containing information for the HKCRthis CLSID) key. If multiple patterns exist, they must be delimited by a semicolon, and numeric subkeys will be generated: 0,1,2...
    pub FileTypeMask: String,
    /// Optional foreign key into the Icon Table, specifying the icon file associated with this CLSID. Will be written under the DefaultIcon key.
    pub Icon_: String,
    /// Optional icon index.
    pub IconIndex: i16,
    /// Optional default inproc handler.  Only optionally provided if Context=CLSCTX_LOCAL_SERVER.  Typically "ole32.dll" or "mapi32.dll"
    pub DefInprocHandler: String,
    /// optional argument for LocalServers.
    pub Argument: String,
    /// Required foreign key into the Feature Table, specifying the feature to validate or install in order for the CLSID factory to be operational.
    pub Feature_: String,
    /// Class registration attributes.
    pub Attributes: i16,
}

impl Class {
    #[rustfmt::skip]
    pub fn new(CLSID: impl Into<String>, Context: impl Into<String>, Component_: impl Into<String>, ProgId_Default: impl Into<String>, Description: impl Into<String>, AppId_: impl Into<String>, FileTypeMask: impl Into<String>, Icon_: impl Into<String>, IconIndex: impl Into<i16>, DefInprocHandler: impl Into<String>, Argument: impl Into<String>, Feature_: impl Into<String>, Attributes: impl Into<i16>) -> Self {
        Self {
            CLSID: CLSID.into(),
            Context: Context.into(),
            Component_: Component_.into(),
            ProgId_Default: ProgId_Default.into(),
            Description: Description.into(),
            AppId_: AppId_.into(),
            FileTypeMask: FileTypeMask.into(),
            Icon_: Icon_.into(),
            IconIndex: IconIndex.into(),
            DefInprocHandler: DefInprocHandler.into(),
            Argument: Argument.into(),
            Feature_: Feature_.into(),
            Attributes: Attributes.into(),
        }
    }
}

impl TableRow for Class {
    const NAME: &'static str = "Class";
    const COLUMNS: &'static str = "CLSID\tContext\tComponent_\tProgId_Default\tDescription\tAppId_\tFileTypeMask\tIcon_\tIconIndex\tDefInprocHandler\tArgument\tFeature_\tAttributes";
    const TYPES: &'static str = "s38\ts32\ts72\tS255\tL255\tS38\tS255\tS72\tI2\tS32\tS255\ts38\tI2";
    const META: &'static str = "Class\tCLSID\tContext\tComponent_";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.CLSID.clone()))?;
        write!(writer, "{}\t", escape(self.Context.clone()))?;
        write!(writer, "{}\t", escape(self.Component_.clone()))?;
        write!(writer, "{}\t", escape(self.ProgId_Default.clone()))?;
        write!(writer, "{}\t", escape(self.Description.clone()))?;
        write!(writer, "{}\t", escape(self.AppId_.clone()))?;
        write!(writer, "{}\t", escape(self.FileTypeMask.clone()))?;
        write!(writer, "{}\t", escape(self.Icon_.clone()))?;
        write!(writer, "{}\t", self.IconIndex)?;
        write!(writer, "{}\t", escape(self.DefInprocHandler.clone()))?;
        write!(writer, "{}\t", escape(self.Argument.clone()))?;
        write!(writer, "{}\t", escape(self.Feature_.clone()))?;
        write!(writer, "{}\r\n", self.Attributes)?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            CLSID: unescape(parts[0].to_string()),
            Context: unescape(parts[1].to_string()),
            Component_: unescape(parts[2].to_string()),
            ProgId_Default: unescape(parts[3].to_string()),
            Description: unescape(parts[4].to_string()),
            AppId_: unescape(parts[5].to_string()),
            FileTypeMask: unescape(parts[6].to_string()),
            Icon_: unescape(parts[7].to_string()),
            IconIndex: parts[8].parse().unwrap(),
            DefInprocHandler: unescape(parts[9].to_string()),
            Argument: unescape(parts[10].to_string()),
            Feature_: unescape(parts[11].to_string()),
            Attributes: parts[12].parse().unwrap(),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct ComboBox {
    /// A named property to be tied to this item. All the items tied to the same property become part of the same combobox.
    pub Property: String,
    /// A positive integer used to determine the ordering of the items within one list.The integers do not have to be consecutive.
    pub Order: i16,
    /// The value string associated with this item. Selecting the line will set the associated property to this value.
    pub Value: String,
    /// The visible text to be assigned to the item. Optional. If this entry or the entire column is missing, the text is the same as the value.
    pub Text: String,
}

impl ComboBox {
    #[rustfmt::skip]
    pub fn new(Property: impl Into<String>, Order: impl Into<i16>, Value: impl Into<String>, Text: impl Into<String>) -> Self {
        Self {
            Property: Property.into(),
            Order: Order.into(),
            Value: Value.into(),
            Text: Text.into(),
        }
    }
}

impl TableRow for ComboBox {
    const NAME: &'static str = "ComboBox";
    const COLUMNS: &'static str = "Property\tOrder\tValue\tText";
    const TYPES: &'static str = "s72\ti2\ts64\tL64";
    const META: &'static str = "ComboBox\tProperty\tOrder";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.Property.clone()))?;
        write!(writer, "{}\t", self.Order)?;
        write!(writer, "{}\t", escape(self.Value.clone()))?;
        write!(writer, "{}\r\n", escape(self.Text.clone()))?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            Property: unescape(parts[0].to_string()),
            Order: parts[1].parse().unwrap(),
            Value: unescape(parts[2].to_string()),
            Text: unescape(parts[3].to_string()),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct CompLocator {
    /// The table key. The Signature_ represents a unique file signature and is also the foreign key in the Signature table.
    pub Signature_: String,
    /// A string GUID unique to this component, version, and language.
    pub ComponentId: String,
    /// A boolean value that determines if the registry value is a filename or a directory location.
    pub Type: i16,
}

impl CompLocator {
    #[rustfmt::skip]
    pub fn new(Signature_: impl Into<String>, ComponentId: impl Into<String>, Type: impl Into<i16>) -> Self {
        Self {
            Signature_: Signature_.into(),
            ComponentId: ComponentId.into(),
            Type: Type.into(),
        }
    }
}

impl TableRow for CompLocator {
    const NAME: &'static str = "CompLocator";
    const COLUMNS: &'static str = "Signature_\tComponentId\tType";
    const TYPES: &'static str = "s72\ts38\tI2";
    const META: &'static str = "CompLocator\tSignature_";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.Signature_.clone()))?;
        write!(writer, "{}\t", escape(self.ComponentId.clone()))?;
        write!(writer, "{}\r\n", self.Type)?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            Signature_: unescape(parts[0].to_string()),
            ComponentId: unescape(parts[1].to_string()),
            Type: parts[2].parse().unwrap(),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct Complus {
    /// Foreign key referencing Component that controls the ComPlus component.
    pub Component_: String,
    /// ComPlus component attributes.
    pub ExpType: i16,
}

impl Complus {
    #[rustfmt::skip]
    pub fn new(Component_: impl Into<String>, ExpType: impl Into<i16>) -> Self {
        Self {
            Component_: Component_.into(),
            ExpType: ExpType.into(),
        }
    }
}

impl TableRow for Complus {
    const NAME: &'static str = "Complus";
    const COLUMNS: &'static str = "Component_\tExpType";
    const TYPES: &'static str = "s72\tI2";
    const META: &'static str = "Complus\tComponent_\tExpType";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.Component_.clone()))?;
        write!(writer, "{}\r\n", self.ExpType)?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            Component_: unescape(parts[0].to_string()),
            ExpType: parts[1].parse().unwrap(),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct Component {
    /// Primary key used to identify a particular component record.
    pub Component: String,
    /// A string GUID unique to this component, version, and language.
    pub ComponentId: String,
    /// Required key of a Directory table record. This is actually a property name whose value contains the actual path, set either by the AppSearch action or with the default setting obtained from the Directory table.
    pub Directory_: String,
    /// Remote execution option, one of irsEnum
    pub Attributes: i16,
    /// A conditional statement that will disable this component if the specified condition evaluates to the 'True' state. If a component is disabled, it will not be installed, regardless of the 'Action' state associated with the component.
    pub Condition: String,
    /// Either the primary key into the File table, Registry table, or ODBCDataSource table. This extract path is stored when the component is installed, and is used to detect the presence of the component and to return the path to it.
    pub KeyPath: String,
}

impl Component {
    #[rustfmt::skip]
    pub fn new(Component: impl Into<String>, ComponentId: impl Into<String>, Directory_: impl Into<String>, Attributes: impl Into<i16>, Condition: impl Into<String>, KeyPath: impl Into<String>) -> Self {
        Self {
            Component: Component.into(),
            ComponentId: ComponentId.into(),
            Directory_: Directory_.into(),
            Attributes: Attributes.into(),
            Condition: Condition.into(),
            KeyPath: KeyPath.into(),
        }
    }
}

impl TableRow for Component {
    const NAME: &'static str = "Component";
    const COLUMNS: &'static str = "Component\tComponentId\tDirectory_\tAttributes\tCondition\tKeyPath";
    const TYPES: &'static str = "s72\tS38\ts72\ti2\tS255\tS72";
    const META: &'static str = "Component\tComponent";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.Component.clone()))?;
        write!(writer, "{}\t", escape(self.ComponentId.clone()))?;
        write!(writer, "{}\t", escape(self.Directory_.clone()))?;
        write!(writer, "{}\t", self.Attributes)?;
        write!(writer, "{}\t", escape(self.Condition.clone()))?;
        write!(writer, "{}\r\n", escape(self.KeyPath.clone()))?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            Component: unescape(parts[0].to_string()),
            ComponentId: unescape(parts[1].to_string()),
            Directory_: unescape(parts[2].to_string()),
            Attributes: parts[3].parse().unwrap(),
            Condition: unescape(parts[4].to_string()),
            KeyPath: unescape(parts[5].to_string()),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct Condition {
    /// Reference to a Feature entry in Feature table.
    pub Feature_: String,
    /// New selection Level to set in Feature table if Condition evaluates to TRUE.
    pub Level: i16,
    /// Expression evaluated to determine if Level in the Feature table is to change.
    pub Condition: String,
}

impl Condition {
    #[rustfmt::skip]
    pub fn new(Feature_: impl Into<String>, Level: impl Into<i16>, Condition: impl Into<String>) -> Self {
        Self {
            Feature_: Feature_.into(),
            Level: Level.into(),
            Condition: Condition.into(),
        }
    }
}

impl TableRow for Condition {
    const NAME: &'static str = "Condition";
    const COLUMNS: &'static str = "Feature_\tLevel\tCondition";
    const TYPES: &'static str = "s38\ti2\tS255";
    const META: &'static str = "Condition\tFeature_\tLevel";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.Feature_.clone()))?;
        write!(writer, "{}\t", self.Level)?;
        write!(writer, "{}\r\n", escape(self.Condition.clone()))?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            Feature_: unescape(parts[0].to_string()),
            Level: parts[1].parse().unwrap(),
            Condition: unescape(parts[2].to_string()),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct Control {
    /// External key to the Dialog table, name of the dialog.
    pub Dialog_: String,
    /// Name of the control. This name must be unique within a dialog, but can repeat on different dialogs. 
    pub Control: String,
    /// The type of the control.
    pub Type: String,
    /// Horizontal coordinate of the upper left corner of the bounding rectangle of the control.
    pub X: i16,
    /// Vertical coordinate of the upper left corner of the bounding rectangle of the control.
    pub Y: i16,
    /// Width of the bounding rectangle of the control.
    pub Width: i16,
    /// Height of the bounding rectangle of the control.
    pub Height: i16,
    /// A 32-bit word that specifies the attribute flags to be applied to this control.
    pub Attributes: i32,
    /// The name of a defined property to be linked to this control. 
    pub Property: String,
    /// A string used to set the initial text contained within a control (if appropriate).
    pub Text: String,
    /// The name of an other control on the same dialog. This link defines the tab order of the controls. The links have to form one or more cycles!
    pub Control_Next: String,
    /// The help strings used with the button. The text is optional. 
    pub Help: String,
}

impl Control {
    #[rustfmt::skip]
    pub fn new(Dialog_: impl Into<String>, Control: impl Into<String>, Type: impl Into<String>, X: impl Into<i16>, Y: impl Into<i16>, Width: impl Into<i16>, Height: impl Into<i16>, Attributes: impl Into<i32>, Property: impl Into<String>, Text: impl Into<String>, Control_Next: impl Into<String>, Help: impl Into<String>) -> Self {
        Self {
            Dialog_: Dialog_.into(),
            Control: Control.into(),
            Type: Type.into(),
            X: X.into(),
            Y: Y.into(),
            Width: Width.into(),
            Height: Height.into(),
            Attributes: Attributes.into(),
            Property: Property.into(),
            Text: Text.into(),
            Control_Next: Control_Next.into(),
            Help: Help.into(),
        }
    }
}

impl TableRow for Control {
    const NAME: &'static str = "Control";
    const COLUMNS: &'static str = "Dialog_\tControl\tType\tX\tY\tWidth\tHeight\tAttributes\tProperty\tText\tControl_Next\tHelp";
    const TYPES: &'static str = "s72\ts50\ts20\ti2\ti2\ti2\ti2\tI4\tS50\tL0\tS50\tL50";
    const META: &'static str = "Control\tDialog_\tControl";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.Dialog_.clone()))?;
        write!(writer, "{}\t", escape(self.Control.clone()))?;
        write!(writer, "{}\t", escape(self.Type.clone()))?;
        write!(writer, "{}\t", self.X)?;
        write!(writer, "{}\t", self.Y)?;
        write!(writer, "{}\t", self.Width)?;
        write!(writer, "{}\t", self.Height)?;
        write!(writer, "{}\t", self.Attributes)?;
        write!(writer, "{}\t", escape(self.Property.clone()))?;
        write!(writer, "{}\t", escape(self.Text.clone()))?;
        write!(writer, "{}\t", escape(self.Control_Next.clone()))?;
        write!(writer, "{}\r\n", escape(self.Help.clone()))?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            Dialog_: unescape(parts[0].to_string()),
            Control: unescape(parts[1].to_string()),
            Type: unescape(parts[2].to_string()),
            X: parts[3].parse().unwrap(),
            Y: parts[4].parse().unwrap(),
            Width: parts[5].parse().unwrap(),
            Height: parts[6].parse().unwrap(),
            Attributes: parts[7].parse().unwrap(),
            Property: unescape(parts[8].to_string()),
            Text: unescape(parts[9].to_string()),
            Control_Next: unescape(parts[10].to_string()),
            Help: unescape(parts[11].to_string()),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct ControlCondition {
    /// A foreign key to the Dialog table, name of the dialog.
    pub Dialog_: String,
    /// A foreign key to the Control table, name of the control.
    pub Control_: String,
    /// The desired action to be taken on the specified control.
    pub Action: String,
    /// A standard conditional statement that specifies under which conditions the action should be triggered.
    pub Condition: String,
}

impl ControlCondition {
    #[rustfmt::skip]
    pub fn new(Dialog_: impl Into<String>, Control_: impl Into<String>, Action: impl Into<String>, Condition: impl Into<String>) -> Self {
        Self {
            Dialog_: Dialog_.into(),
            Control_: Control_.into(),
            Action: Action.into(),
            Condition: Condition.into(),
        }
    }
}

impl TableRow for ControlCondition {
    const NAME: &'static str = "ControlCondition";
    const COLUMNS: &'static str = "Dialog_\tControl_\tAction\tCondition";
    const TYPES: &'static str = "s72\ts50\ts50\ts255";
    const META: &'static str = "ControlCondition\tDialog_\tControl_\tAction\tCondition";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.Dialog_.clone()))?;
        write!(writer, "{}\t", escape(self.Control_.clone()))?;
        write!(writer, "{}\t", escape(self.Action.clone()))?;
        write!(writer, "{}\r\n", escape(self.Condition.clone()))?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            Dialog_: unescape(parts[0].to_string()),
            Control_: unescape(parts[1].to_string()),
            Action: unescape(parts[2].to_string()),
            Condition: unescape(parts[3].to_string()),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct ControlEvent {
    /// A foreign key to the Dialog table, name of the dialog.
    pub Dialog_: String,
    /// A foreign key to the Control table, name of the control
    pub Control_: String,
    /// An identifier that specifies the type of the event that should take place when the user interacts with control specified by the first two entries.
    pub Event: String,
    /// A value to be used as a modifier when triggering a particular event.
    pub Argument: String,
    /// A standard conditional statement that specifies under which conditions an event should be triggered.
    pub Condition: String,
    /// An integer used to order several events tied to the same control. Can be left blank.
    pub Ordering: i16,
}

impl ControlEvent {
    #[rustfmt::skip]
    pub fn new(Dialog_: impl Into<String>, Control_: impl Into<String>, Event: impl Into<String>, Argument: impl Into<String>, Condition: impl Into<String>, Ordering: impl Into<i16>) -> Self {
        Self {
            Dialog_: Dialog_.into(),
            Control_: Control_.into(),
            Event: Event.into(),
            Argument: Argument.into(),
            Condition: Condition.into(),
            Ordering: Ordering.into(),
        }
    }
}

impl TableRow for ControlEvent {
    const NAME: &'static str = "ControlEvent";
    const COLUMNS: &'static str = "Dialog_\tControl_\tEvent\tArgument\tCondition\tOrdering";
    const TYPES: &'static str = "s72\ts50\ts50\ts255\tS255\tI2";
    const META: &'static str = "ControlEvent\tDialog_\tControl_\tEvent\tArgument\tCondition";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.Dialog_.clone()))?;
        write!(writer, "{}\t", escape(self.Control_.clone()))?;
        write!(writer, "{}\t", escape(self.Event.clone()))?;
        write!(writer, "{}\t", escape(self.Argument.clone()))?;
        write!(writer, "{}\t", escape(self.Condition.clone()))?;
        write!(writer, "{}\r\n", self.Ordering)?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            Dialog_: unescape(parts[0].to_string()),
            Control_: unescape(parts[1].to_string()),
            Event: unescape(parts[2].to_string()),
            Argument: unescape(parts[3].to_string()),
            Condition: unescape(parts[4].to_string()),
            Ordering: parts[5].parse().unwrap(),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct CreateFolder {
    /// Primary key, could be foreign key into the Directory table.
    pub Directory_: String,
    /// Foreign key into the Component table.
    pub Component_: String,
}

impl CreateFolder {
    #[rustfmt::skip]
    pub fn new(Directory_: impl Into<String>, Component_: impl Into<String>) -> Self {
        Self {
            Directory_: Directory_.into(),
            Component_: Component_.into(),
        }
    }
}

impl TableRow for CreateFolder {
    const NAME: &'static str = "CreateFolder";
    const COLUMNS: &'static str = "Directory_\tComponent_";
    const TYPES: &'static str = "s72\ts72";
    const META: &'static str = "CreateFolder\tDirectory_\tComponent_";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.Directory_.clone()))?;
        write!(writer, "{}\r\n", escape(self.Component_.clone()))?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            Directory_: unescape(parts[0].to_string()),
            Component_: unescape(parts[1].to_string()),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct CustomAction {
    /// Primary key, name of action, normally appears in sequence table unless private use.
    pub Action: String,
    /// The numeric custom action type, consisting of source location, code type, entry, option flags.
    pub Type: i16,
    /// The table reference of the source of the code.
    pub Source: String,
    /// Excecution parameter, depends on the type of custom action
    pub Target: String,
}

impl CustomAction {
    #[rustfmt::skip]
    pub fn new(Action: impl Into<String>, Type: impl Into<i16>, Source: impl Into<String>, Target: impl Into<String>) -> Self {
        Self {
            Action: Action.into(),
            Type: Type.into(),
            Source: Source.into(),
            Target: Target.into(),
        }
    }
}

impl TableRow for CustomAction {
    const NAME: &'static str = "CustomAction";
    const COLUMNS: &'static str = "Action\tType\tSource\tTarget";
    const TYPES: &'static str = "s72\ti2\tS72\tS255";
    const META: &'static str = "CustomAction\tAction";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.Action.clone()))?;
        write!(writer, "{}\t", self.Type)?;
        write!(writer, "{}\t", escape(self.Source.clone()))?;
        write!(writer, "{}\r\n", escape(self.Target.clone()))?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            Action: unescape(parts[0].to_string()),
            Type: parts[1].parse().unwrap(),
            Source: unescape(parts[2].to_string()),
            Target: unescape(parts[3].to_string()),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct Dialog {
    /// Name of the dialog.
    pub Dialog: String,
    /// Horizontal position of the dialog on a 0-100 scale. 0 means left end, 100 means right end of the screen, 50 center.
    pub HCentering: i16,
    /// Vertical position of the dialog on a 0-100 scale. 0 means top end, 100 means bottom end of the screen, 50 center.
    pub VCentering: i16,
    /// Width of the bounding rectangle of the dialog.
    pub Width: i16,
    /// Height of the bounding rectangle of the dialog.
    pub Height: i16,
    /// A 32-bit word that specifies the attribute flags to be applied to this dialog.
    pub Attributes: i32,
    /// A text string specifying the title to be displayed in the title bar of the dialog's window.
    pub Title: String,
    /// Defines the control that has the focus when the dialog is created.
    pub Control_First: String,
    /// Defines the default control. Hitting return is equivalent to pushing this button.
    pub Control_Default: String,
    /// Defines the cancel control. Hitting escape or clicking on the close icon on the dialog is equivalent to pushing this button.
    pub Control_Cancel: String,
}

impl Dialog {
    #[rustfmt::skip]
    pub fn new(Dialog: impl Into<String>, HCentering: impl Into<i16>, VCentering: impl Into<i16>, Width: impl Into<i16>, Height: impl Into<i16>, Attributes: impl Into<i32>, Title: impl Into<String>, Control_First: impl Into<String>, Control_Default: impl Into<String>, Control_Cancel: impl Into<String>) -> Self {
        Self {
            Dialog: Dialog.into(),
            HCentering: HCentering.into(),
            VCentering: VCentering.into(),
            Width: Width.into(),
            Height: Height.into(),
            Attributes: Attributes.into(),
            Title: Title.into(),
            Control_First: Control_First.into(),
            Control_Default: Control_Default.into(),
            Control_Cancel: Control_Cancel.into(),
        }
    }
}

impl TableRow for Dialog {
    const NAME: &'static str = "Dialog";
    const COLUMNS: &'static str = "Dialog\tHCentering\tVCentering\tWidth\tHeight\tAttributes\tTitle\tControl_First\tControl_Default\tControl_Cancel";
    const TYPES: &'static str = "s72\ti2\ti2\ti2\ti2\tI4\tL128\ts50\tS50\tS50";
    const META: &'static str = "Dialog\tDialog";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.Dialog.clone()))?;
        write!(writer, "{}\t", self.HCentering)?;
        write!(writer, "{}\t", self.VCentering)?;
        write!(writer, "{}\t", self.Width)?;
        write!(writer, "{}\t", self.Height)?;
        write!(writer, "{}\t", self.Attributes)?;
        write!(writer, "{}\t", escape(self.Title.clone()))?;
        write!(writer, "{}\t", escape(self.Control_First.clone()))?;
        write!(writer, "{}\t", escape(self.Control_Default.clone()))?;
        write!(writer, "{}\r\n", escape(self.Control_Cancel.clone()))?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            Dialog: unescape(parts[0].to_string()),
            HCentering: parts[1].parse().unwrap(),
            VCentering: parts[2].parse().unwrap(),
            Width: parts[3].parse().unwrap(),
            Height: parts[4].parse().unwrap(),
            Attributes: parts[5].parse().unwrap(),
            Title: unescape(parts[6].to_string()),
            Control_First: unescape(parts[7].to_string()),
            Control_Default: unescape(parts[8].to_string()),
            Control_Cancel: unescape(parts[9].to_string()),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct Directory {
    /// Unique identifier for directory entry, primary key. If a property by this name is defined, it contains the full path to the directory.
    pub Directory: String,
    /// Reference to the entry in this table specifying the default parent directory. A record parented to itself or with a Null parent represents a root of the install tree.
    pub Directory_Parent: String,
    /// The default sub-path under parent's path.
    pub DefaultDir: String,
}

impl Directory {
    #[rustfmt::skip]
    pub fn new(Directory: impl Into<String>, Directory_Parent: impl Into<String>, DefaultDir: impl Into<String>) -> Self {
        Self {
            Directory: Directory.into(),
            Directory_Parent: Directory_Parent.into(),
            DefaultDir: DefaultDir.into(),
        }
    }
}

impl TableRow for Directory {
    const NAME: &'static str = "Directory";
    const COLUMNS: &'static str = "Directory\tDirectory_Parent\tDefaultDir";
    const TYPES: &'static str = "s72\tS72\tl255";
    const META: &'static str = "Directory\tDirectory";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.Directory.clone()))?;
        write!(writer, "{}\t", escape(self.Directory_Parent.clone()))?;
        write!(writer, "{}\r\n", escape(self.DefaultDir.clone()))?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            Directory: unescape(parts[0].to_string()),
            Directory_Parent: unescape(parts[1].to_string()),
            DefaultDir: unescape(parts[2].to_string()),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct DrLocator {
    /// The Signature_ represents a unique file signature and is also the foreign key in the Signature table.
    pub Signature_: String,
    /// The parent file signature. It is also a foreign key in the Signature table. If null and the Path column does not expand to a full path, then all the fixed drives of the user system are searched using the Path.
    pub Parent: String,
    /// The path on the user system. This is a either a subpath below the value of the Parent or a full path. The path may contain properties enclosed within [ ] that will be expanded.
    pub Path: String,
    /// The depth below the path to which the Signature_ is recursively searched. If absent, the depth is assumed to be 0.
    pub Depth: i16,
}

impl DrLocator {
    #[rustfmt::skip]
    pub fn new(Signature_: impl Into<String>, Parent: impl Into<String>, Path: impl Into<String>, Depth: impl Into<i16>) -> Self {
        Self {
            Signature_: Signature_.into(),
            Parent: Parent.into(),
            Path: Path.into(),
            Depth: Depth.into(),
        }
    }
}

impl TableRow for DrLocator {
    const NAME: &'static str = "DrLocator";
    const COLUMNS: &'static str = "Signature_\tParent\tPath\tDepth";
    const TYPES: &'static str = "s72\tS72\tS255\tI2";
    const META: &'static str = "DrLocator\tSignature_\tParent\tPath";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.Signature_.clone()))?;
        write!(writer, "{}\t", escape(self.Parent.clone()))?;
        write!(writer, "{}\t", escape(self.Path.clone()))?;
        write!(writer, "{}\r\n", self.Depth)?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            Signature_: unescape(parts[0].to_string()),
            Parent: unescape(parts[1].to_string()),
            Path: unescape(parts[2].to_string()),
            Depth: parts[3].parse().unwrap(),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct DuplicateFile {
    /// Primary key used to identify a particular file entry
    pub FileKey: String,
    /// Foreign key referencing Component that controls the duplicate file.
    pub Component_: String,
    /// Foreign key referencing the source file to be duplicated.
    pub File_: String,
    /// Filename to be given to the duplicate file.
    pub DestName: String,
    /// Name of a property whose value is assumed to resolve to the full pathname to a destination folder.
    pub DestFolder: String,
}

impl DuplicateFile {
    #[rustfmt::skip]
    pub fn new(FileKey: impl Into<String>, Component_: impl Into<String>, File_: impl Into<String>, DestName: impl Into<String>, DestFolder: impl Into<String>) -> Self {
        Self {
            FileKey: FileKey.into(),
            Component_: Component_.into(),
            File_: File_.into(),
            DestName: DestName.into(),
            DestFolder: DestFolder.into(),
        }
    }
}

impl TableRow for DuplicateFile {
    const NAME: &'static str = "DuplicateFile";
    const COLUMNS: &'static str = "FileKey\tComponent_\tFile_\tDestName\tDestFolder";
    const TYPES: &'static str = "s72\ts72\ts72\tL255\tS72";
    const META: &'static str = "DuplicateFile\tFileKey";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.FileKey.clone()))?;
        write!(writer, "{}\t", escape(self.Component_.clone()))?;
        write!(writer, "{}\t", escape(self.File_.clone()))?;
        write!(writer, "{}\t", escape(self.DestName.clone()))?;
        write!(writer, "{}\r\n", escape(self.DestFolder.clone()))?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            FileKey: unescape(parts[0].to_string()),
            Component_: unescape(parts[1].to_string()),
            File_: unescape(parts[2].to_string()),
            DestName: unescape(parts[3].to_string()),
            DestFolder: unescape(parts[4].to_string()),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct Environment {
    /// Unique identifier for the environmental variable setting
    pub Environment: String,
    /// The name of the environmental value.
    pub Name: String,
    /// The value to set in the environmental settings.
    pub Value: String,
    /// Foreign key into the Component table referencing component that controls the installing of the environmental value.
    pub Component_: String,
}

impl Environment {
    #[rustfmt::skip]
    pub fn new(Environment: impl Into<String>, Name: impl Into<String>, Value: impl Into<String>, Component_: impl Into<String>) -> Self {
        Self {
            Environment: Environment.into(),
            Name: Name.into(),
            Value: Value.into(),
            Component_: Component_.into(),
        }
    }
}

impl TableRow for Environment {
    const NAME: &'static str = "Environment";
    const COLUMNS: &'static str = "Environment\tName\tValue\tComponent_";
    const TYPES: &'static str = "s72\tl255\tL255\ts72";
    const META: &'static str = "Environment\tEnvironment";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.Environment.clone()))?;
        write!(writer, "{}\t", escape(self.Name.clone()))?;
        write!(writer, "{}\t", escape(self.Value.clone()))?;
        write!(writer, "{}\r\n", escape(self.Component_.clone()))?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            Environment: unescape(parts[0].to_string()),
            Name: unescape(parts[1].to_string()),
            Value: unescape(parts[2].to_string()),
            Component_: unescape(parts[3].to_string()),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct Error {
    /// Integer error number, obtained from header file IError(...) macros.
    pub Error: i16,
    /// Error formatting template, obtained from user ed. or localizers.
    pub Message: String,
}

impl Error {
    #[rustfmt::skip]
    pub fn new(Error: impl Into<i16>, Message: impl Into<String>) -> Self {
        Self {
            Error: Error.into(),
            Message: Message.into(),
        }
    }
}

impl TableRow for Error {
    const NAME: &'static str = "Error";
    const COLUMNS: &'static str = "Error\tMessage";
    const TYPES: &'static str = "i2\tL0";
    const META: &'static str = "Error\tError";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", self.Error)?;
        write!(writer, "{}\r\n", escape(self.Message.clone()))?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            Error: parts[0].parse().unwrap(),
            Message: unescape(parts[1].to_string()),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct EventMapping {
    /// A foreign key to the Dialog table, name of the Dialog.
    pub Dialog_: String,
    /// A foreign key to the Control table, name of the control.
    pub Control_: String,
    /// An identifier that specifies the type of the event that the control subscribes to.
    pub Event: String,
    /// The name of the control attribute, that is set when this event is received.
    pub Attribute: String,
}

impl EventMapping {
    #[rustfmt::skip]
    pub fn new(Dialog_: impl Into<String>, Control_: impl Into<String>, Event: impl Into<String>, Attribute: impl Into<String>) -> Self {
        Self {
            Dialog_: Dialog_.into(),
            Control_: Control_.into(),
            Event: Event.into(),
            Attribute: Attribute.into(),
        }
    }
}

impl TableRow for EventMapping {
    const NAME: &'static str = "EventMapping";
    const COLUMNS: &'static str = "Dialog_\tControl_\tEvent\tAttribute";
    const TYPES: &'static str = "s72\ts50\ts50\ts50";
    const META: &'static str = "EventMapping\tDialog_\tControl_\tEvent";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.Dialog_.clone()))?;
        write!(writer, "{}\t", escape(self.Control_.clone()))?;
        write!(writer, "{}\t", escape(self.Event.clone()))?;
        write!(writer, "{}\r\n", escape(self.Attribute.clone()))?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            Dialog_: unescape(parts[0].to_string()),
            Control_: unescape(parts[1].to_string()),
            Event: unescape(parts[2].to_string()),
            Attribute: unescape(parts[3].to_string()),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct Extension {
    /// The extension associated with the table row.
    pub Extension: String,
    /// Required foreign key into the Component Table, specifying the component for which to return a path when called through LocateComponent.
    pub Component_: String,
    /// Optional ProgId associated with this extension.
    pub ProgId_: String,
    /// Optional Context identifier, typically "type/format" associated with the extension
    pub MIME_: String,
    /// Required foreign key into the Feature Table, specifying the feature to validate or install in order for the CLSID factory to be operational.
    pub Feature_: String,
}

impl Extension {
    #[rustfmt::skip]
    pub fn new(Extension: impl Into<String>, Component_: impl Into<String>, ProgId_: impl Into<String>, MIME_: impl Into<String>, Feature_: impl Into<String>) -> Self {
        Self {
            Extension: Extension.into(),
            Component_: Component_.into(),
            ProgId_: ProgId_.into(),
            MIME_: MIME_.into(),
            Feature_: Feature_.into(),
        }
    }
}

impl TableRow for Extension {
    const NAME: &'static str = "Extension";
    const COLUMNS: &'static str = "Extension\tComponent_\tProgId_\tMIME_\tFeature_";
    const TYPES: &'static str = "s255\ts72\tS255\tS64\ts38";
    const META: &'static str = "Extension\tExtension\tComponent_";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.Extension.clone()))?;
        write!(writer, "{}\t", escape(self.Component_.clone()))?;
        write!(writer, "{}\t", escape(self.ProgId_.clone()))?;
        write!(writer, "{}\t", escape(self.MIME_.clone()))?;
        write!(writer, "{}\r\n", escape(self.Feature_.clone()))?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            Extension: unescape(parts[0].to_string()),
            Component_: unescape(parts[1].to_string()),
            ProgId_: unescape(parts[2].to_string()),
            MIME_: unescape(parts[3].to_string()),
            Feature_: unescape(parts[4].to_string()),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct Feature {
    /// Primary key used to identify a particular feature record.
    pub Feature: String,
    /// Optional key of a parent record in the same table. If the parent is not selected, then the record will not be installed. Null indicates a root item.
    pub Feature_Parent: String,
    /// Short text identifying a visible feature item.
    pub Title: String,
    /// Longer descriptive text describing a visible feature item.
    pub Description: String,
    /// Numeric sort order, used to force a specific display ordering.
    pub Display: i16,
    /// The install level at which record will be initially selected. An install level of 0 will disable an item and prevent its display.
    pub Level: i16,
    /// The name of the Directory that can be configured by the UI. A non-null value will enable the browse button.
    pub Directory_: String,
    /// Feature attributes
    pub Attributes: i16,
}

impl Feature {
    #[rustfmt::skip]
    pub fn new(Feature: impl Into<String>, Feature_Parent: impl Into<String>, Title: impl Into<String>, Description: impl Into<String>, Display: impl Into<i16>, Level: impl Into<i16>, Directory_: impl Into<String>, Attributes: impl Into<i16>) -> Self {
        Self {
            Feature: Feature.into(),
            Feature_Parent: Feature_Parent.into(),
            Title: Title.into(),
            Description: Description.into(),
            Display: Display.into(),
            Level: Level.into(),
            Directory_: Directory_.into(),
            Attributes: Attributes.into(),
        }
    }
}

impl TableRow for Feature {
    const NAME: &'static str = "Feature";
    const COLUMNS: &'static str = "Feature\tFeature_Parent\tTitle\tDescription\tDisplay\tLevel\tDirectory_\tAttributes";
    const TYPES: &'static str = "s38\tS38\tL64\tL255\tI2\ti2\tS72\ti2";
    const META: &'static str = "Feature\tFeature";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.Feature.clone()))?;
        write!(writer, "{}\t", escape(self.Feature_Parent.clone()))?;
        write!(writer, "{}\t", escape(self.Title.clone()))?;
        write!(writer, "{}\t", escape(self.Description.clone()))?;
        write!(writer, "{}\t", self.Display)?;
        write!(writer, "{}\t", self.Level)?;
        write!(writer, "{}\t", escape(self.Directory_.clone()))?;
        write!(writer, "{}\r\n", self.Attributes)?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            Feature: unescape(parts[0].to_string()),
            Feature_Parent: unescape(parts[1].to_string()),
            Title: unescape(parts[2].to_string()),
            Description: unescape(parts[3].to_string()),
            Display: parts[4].parse().unwrap(),
            Level: parts[5].parse().unwrap(),
            Directory_: unescape(parts[6].to_string()),
            Attributes: parts[7].parse().unwrap(),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct FeatureComponents {
    /// Foreign key into Feature table.
    pub Feature_: String,
    /// Foreign key into Component table.
    pub Component_: String,
}

impl FeatureComponents {
    #[rustfmt::skip]
    pub fn new(Feature_: impl Into<String>, Component_: impl Into<String>) -> Self {
        Self {
            Feature_: Feature_.into(),
            Component_: Component_.into(),
        }
    }
}

impl TableRow for FeatureComponents {
    const NAME: &'static str = "FeatureComponents";
    const COLUMNS: &'static str = "Feature_\tComponent_";
    const TYPES: &'static str = "s38\ts72";
    const META: &'static str = "FeatureComponents\tFeature_\tComponent_";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.Feature_.clone()))?;
        write!(writer, "{}\r\n", escape(self.Component_.clone()))?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            Feature_: unescape(parts[0].to_string()),
            Component_: unescape(parts[1].to_string()),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct File {
    /// Primary key, non-localized token, must match identifier in cabinet.  For uncompressed files, this field is ignored.
    pub File: String,
    /// Foreign key referencing Component that controls the file.
    pub Component_: String,
    /// File name used for installation, may be localized.  This may contain a "short name|long name" pair.
    pub FileName: String,
    /// Size of file in bytes (long integer).
    pub FileSize: i32,
    /// Version string for versioned files;  Blank for unversioned files.
    pub Version: String,
    /// List of decimal language Ids, comma-separated if more than one.
    pub Language: String,
    /// Integer containing bit flags representing file attributes (with the decimal value of each bit position in parentheses)
    pub Attributes: i16,
    /// Sequence with respect to the media images; order must track cabinet order.
    pub Sequence: i16,
}

impl File {
    #[rustfmt::skip]
    pub fn new(File: impl Into<String>, Component_: impl Into<String>, FileName: impl Into<String>, FileSize: impl Into<i32>, Version: impl Into<String>, Language: impl Into<String>, Attributes: impl Into<i16>, Sequence: impl Into<i16>) -> Self {
        Self {
            File: File.into(),
            Component_: Component_.into(),
            FileName: FileName.into(),
            FileSize: FileSize.into(),
            Version: Version.into(),
            Language: Language.into(),
            Attributes: Attributes.into(),
            Sequence: Sequence.into(),
        }
    }
}

impl TableRow for File {
    const NAME: &'static str = "File";
    const COLUMNS: &'static str = "File\tComponent_\tFileName\tFileSize\tVersion\tLanguage\tAttributes\tSequence";
    const TYPES: &'static str = "s72\ts72\tl255\ti4\tS72\tS20\tI2\ti2";
    const META: &'static str = "File\tFile";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.File.clone()))?;
        write!(writer, "{}\t", escape(self.Component_.clone()))?;
        write!(writer, "{}\t", escape(self.FileName.clone()))?;
        write!(writer, "{}\t", self.FileSize)?;
        write!(writer, "{}\t", escape(self.Version.clone()))?;
        write!(writer, "{}\t", escape(self.Language.clone()))?;
        write!(writer, "{}\t", self.Attributes)?;
        write!(writer, "{}\r\n", self.Sequence)?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            File: unescape(parts[0].to_string()),
            Component_: unescape(parts[1].to_string()),
            FileName: unescape(parts[2].to_string()),
            FileSize: parts[3].parse().unwrap(),
            Version: unescape(parts[4].to_string()),
            Language: unescape(parts[5].to_string()),
            Attributes: parts[6].parse().unwrap(),
            Sequence: parts[7].parse().unwrap(),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct FileSFPCatalog {
    /// File associated with the catalog
    pub File_: String,
    /// Catalog associated with the file
    pub SFPCatalog_: String,
}

impl FileSFPCatalog {
    #[rustfmt::skip]
    pub fn new(File_: impl Into<String>, SFPCatalog_: impl Into<String>) -> Self {
        Self {
            File_: File_.into(),
            SFPCatalog_: SFPCatalog_.into(),
        }
    }
}

impl TableRow for FileSFPCatalog {
    const NAME: &'static str = "FileSFPCatalog";
    const COLUMNS: &'static str = "File_\tSFPCatalog_";
    const TYPES: &'static str = "s72\ts255";
    const META: &'static str = "FileSFPCatalog\tFile_\tSFPCatalog_";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.File_.clone()))?;
        write!(writer, "{}\r\n", escape(self.SFPCatalog_.clone()))?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            File_: unescape(parts[0].to_string()),
            SFPCatalog_: unescape(parts[1].to_string()),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct Font {
    /// Primary key, foreign key into File table referencing font file.
    pub File_: String,
    /// Font name.
    pub FontTitle: String,
}

impl Font {
    #[rustfmt::skip]
    pub fn new(File_: impl Into<String>, FontTitle: impl Into<String>) -> Self {
        Self {
            File_: File_.into(),
            FontTitle: FontTitle.into(),
        }
    }
}

impl TableRow for Font {
    const NAME: &'static str = "Font";
    const COLUMNS: &'static str = "File_\tFontTitle";
    const TYPES: &'static str = "s72\tS128";
    const META: &'static str = "Font\tFile_";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.File_.clone()))?;
        write!(writer, "{}\r\n", escape(self.FontTitle.clone()))?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            File_: unescape(parts[0].to_string()),
            FontTitle: unescape(parts[1].to_string()),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct IniFile {
    /// Primary key, non-localized token.
    pub IniFile: String,
    /// The .INI file name in which to write the information
    pub FileName: String,
    /// Foreign key into the Directory table denoting the directory where the .INI file is.
    pub DirProperty: String,
    /// The .INI file Section.
    pub Section: String,
    /// The .INI file key below Section.
    pub Key: String,
    /// The value to be written.
    pub Value: String,
    /// The type of modification to be made, one of iifEnum
    pub Action: i16,
    /// Foreign key into the Component table referencing component that controls the installing of the .INI value.
    pub Component_: String,
}

impl IniFile {
    #[rustfmt::skip]
    pub fn new(IniFile: impl Into<String>, FileName: impl Into<String>, DirProperty: impl Into<String>, Section: impl Into<String>, Key: impl Into<String>, Value: impl Into<String>, Action: impl Into<i16>, Component_: impl Into<String>) -> Self {
        Self {
            IniFile: IniFile.into(),
            FileName: FileName.into(),
            DirProperty: DirProperty.into(),
            Section: Section.into(),
            Key: Key.into(),
            Value: Value.into(),
            Action: Action.into(),
            Component_: Component_.into(),
        }
    }
}

impl TableRow for IniFile {
    const NAME: &'static str = "IniFile";
    const COLUMNS: &'static str = "IniFile\tFileName\tDirProperty\tSection\tKey\tValue\tAction\tComponent_";
    const TYPES: &'static str = "s72\tl255\tS72\tl96\tl128\tl255\ti2\ts72";
    const META: &'static str = "IniFile\tIniFile";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.IniFile.clone()))?;
        write!(writer, "{}\t", escape(self.FileName.clone()))?;
        write!(writer, "{}\t", escape(self.DirProperty.clone()))?;
        write!(writer, "{}\t", escape(self.Section.clone()))?;
        write!(writer, "{}\t", escape(self.Key.clone()))?;
        write!(writer, "{}\t", escape(self.Value.clone()))?;
        write!(writer, "{}\t", self.Action)?;
        write!(writer, "{}\r\n", escape(self.Component_.clone()))?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            IniFile: unescape(parts[0].to_string()),
            FileName: unescape(parts[1].to_string()),
            DirProperty: unescape(parts[2].to_string()),
            Section: unescape(parts[3].to_string()),
            Key: unescape(parts[4].to_string()),
            Value: unescape(parts[5].to_string()),
            Action: parts[6].parse().unwrap(),
            Component_: unescape(parts[7].to_string()),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct IniLocator {
    /// The table key. The Signature_ represents a unique file signature and is also the foreign key in the Signature table.
    pub Signature_: String,
    /// The .INI file name.
    pub FileName: String,
    /// Section name within in file (within square brackets in INI file).
    pub Section: String,
    /// Key value (followed by an equals sign in INI file).
    pub Key: String,
    /// The field in the .INI line. If Field is null or 0 the entire line is read.
    pub Field: i16,
    /// An integer value that determines if the .INI value read is a filename or a directory location or to be used as is w/o interpretation.
    pub Type: i16,
}

impl IniLocator {
    #[rustfmt::skip]
    pub fn new(Signature_: impl Into<String>, FileName: impl Into<String>, Section: impl Into<String>, Key: impl Into<String>, Field: impl Into<i16>, Type: impl Into<i16>) -> Self {
        Self {
            Signature_: Signature_.into(),
            FileName: FileName.into(),
            Section: Section.into(),
            Key: Key.into(),
            Field: Field.into(),
            Type: Type.into(),
        }
    }
}

impl TableRow for IniLocator {
    const NAME: &'static str = "IniLocator";
    const COLUMNS: &'static str = "Signature_\tFileName\tSection\tKey\tField\tType";
    const TYPES: &'static str = "s72\ts255\ts96\ts128\tI2\tI2";
    const META: &'static str = "IniLocator\tSignature_";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.Signature_.clone()))?;
        write!(writer, "{}\t", escape(self.FileName.clone()))?;
        write!(writer, "{}\t", escape(self.Section.clone()))?;
        write!(writer, "{}\t", escape(self.Key.clone()))?;
        write!(writer, "{}\t", self.Field)?;
        write!(writer, "{}\r\n", self.Type)?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            Signature_: unescape(parts[0].to_string()),
            FileName: unescape(parts[1].to_string()),
            Section: unescape(parts[2].to_string()),
            Key: unescape(parts[3].to_string()),
            Field: parts[4].parse().unwrap(),
            Type: parts[5].parse().unwrap(),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct InstallExecuteSequence {
    /// Name of action to invoke, either in the engine or the handler DLL.
    pub Action: String,
    /// Optional expression which skips the action if evaluates to expFalse.If the expression syntax is invalid, the engine will terminate, returning iesBadActionData.
    pub Condition: String,
    /// Number that determines the sort order in which the actions are to be executed.  Leave blank to suppress action.
    pub Sequence: i16,
}

impl InstallExecuteSequence {
    #[rustfmt::skip]
    pub fn new(Action: impl Into<String>, Condition: impl Into<String>, Sequence: impl Into<i16>) -> Self {
        Self {
            Action: Action.into(),
            Condition: Condition.into(),
            Sequence: Sequence.into(),
        }
    }
}

impl TableRow for InstallExecuteSequence {
    const NAME: &'static str = "InstallExecuteSequence";
    const COLUMNS: &'static str = "Action\tCondition\tSequence";
    const TYPES: &'static str = "s72\tS255\tI2";
    const META: &'static str = "InstallExecuteSequence\tAction";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.Action.clone()))?;
        write!(writer, "{}\t", escape(self.Condition.clone()))?;
        write!(writer, "{}\r\n", self.Sequence)?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            Action: unescape(parts[0].to_string()),
            Condition: unescape(parts[1].to_string()),
            Sequence: parts[2].parse().unwrap(),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct InstallUISequence {
    /// Name of action to invoke, either in the engine or the handler DLL.
    pub Action: String,
    /// Optional expression which skips the action if evaluates to expFalse.If the expression syntax is invalid, the engine will terminate, returning iesBadActionData.
    pub Condition: String,
    /// Number that determines the sort order in which the actions are to be executed.  Leave blank to suppress action.
    pub Sequence: i16,
}

impl InstallUISequence {
    #[rustfmt::skip]
    pub fn new(Action: impl Into<String>, Condition: impl Into<String>, Sequence: impl Into<i16>) -> Self {
        Self {
            Action: Action.into(),
            Condition: Condition.into(),
            Sequence: Sequence.into(),
        }
    }
}

impl TableRow for InstallUISequence {
    const NAME: &'static str = "InstallUISequence";
    const COLUMNS: &'static str = "Action\tCondition\tSequence";
    const TYPES: &'static str = "s72\tS255\tI2";
    const META: &'static str = "InstallUISequence\tAction";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.Action.clone()))?;
        write!(writer, "{}\t", escape(self.Condition.clone()))?;
        write!(writer, "{}\r\n", self.Sequence)?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            Action: unescape(parts[0].to_string()),
            Condition: unescape(parts[1].to_string()),
            Sequence: parts[2].parse().unwrap(),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct IsolatedComponent {
    /// Key to Component table item to be isolated
    pub Component_Shared: String,
    /// Key to Component table item for application
    pub Component_Application: String,
}

impl IsolatedComponent {
    #[rustfmt::skip]
    pub fn new(Component_Shared: impl Into<String>, Component_Application: impl Into<String>) -> Self {
        Self {
            Component_Shared: Component_Shared.into(),
            Component_Application: Component_Application.into(),
        }
    }
}

impl TableRow for IsolatedComponent {
    const NAME: &'static str = "IsolatedComponent";
    const COLUMNS: &'static str = "Component_Shared\tComponent_Application";
    const TYPES: &'static str = "s72\ts72";
    const META: &'static str = "IsolatedComponent\tComponent_Shared\tComponent_Application";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.Component_Shared.clone()))?;
        write!(writer, "{}\r\n", escape(self.Component_Application.clone()))?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            Component_Shared: unescape(parts[0].to_string()),
            Component_Application: unescape(parts[1].to_string()),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct LaunchCondition {
    /// Expression which must evaluate to TRUE in order for install to commence.
    pub Condition: String,
    /// Localizable text to display when condition fails and install must abort.
    pub Description: String,
}

impl LaunchCondition {
    #[rustfmt::skip]
    pub fn new(Condition: impl Into<String>, Description: impl Into<String>) -> Self {
        Self {
            Condition: Condition.into(),
            Description: Description.into(),
        }
    }
}

impl TableRow for LaunchCondition {
    const NAME: &'static str = "LaunchCondition";
    const COLUMNS: &'static str = "Condition\tDescription";
    const TYPES: &'static str = "s255\tl255";
    const META: &'static str = "LaunchCondition\tCondition";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.Condition.clone()))?;
        write!(writer, "{}\r\n", escape(self.Description.clone()))?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            Condition: unescape(parts[0].to_string()),
            Description: unescape(parts[1].to_string()),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct ListBox {
    /// A named property to be tied to this item. All the items tied to the same property become part of the same listbox.
    pub Property: String,
    /// A positive integer used to determine the ordering of the items within one list..The integers do not have to be consecutive.
    pub Order: i16,
    /// The value string associated with this item. Selecting the line will set the associated property to this value.
    pub Value: String,
    /// The visible text to be assigned to the item. Optional. If this entry or the entire column is missing, the text is the same as the value.
    pub Text: String,
}

impl ListBox {
    #[rustfmt::skip]
    pub fn new(Property: impl Into<String>, Order: impl Into<i16>, Value: impl Into<String>, Text: impl Into<String>) -> Self {
        Self {
            Property: Property.into(),
            Order: Order.into(),
            Value: Value.into(),
            Text: Text.into(),
        }
    }
}

impl TableRow for ListBox {
    const NAME: &'static str = "ListBox";
    const COLUMNS: &'static str = "Property\tOrder\tValue\tText";
    const TYPES: &'static str = "s72\ti2\ts64\tL64";
    const META: &'static str = "ListBox\tProperty\tOrder";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.Property.clone()))?;
        write!(writer, "{}\t", self.Order)?;
        write!(writer, "{}\t", escape(self.Value.clone()))?;
        write!(writer, "{}\r\n", escape(self.Text.clone()))?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            Property: unescape(parts[0].to_string()),
            Order: parts[1].parse().unwrap(),
            Value: unescape(parts[2].to_string()),
            Text: unescape(parts[3].to_string()),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct ListView {
    /// A named property to be tied to this item. All the items tied to the same property become part of the same listview.
    pub Property: String,
    /// A positive integer used to determine the ordering of the items within one list..The integers do not have to be consecutive.
    pub Order: i16,
    /// The value string associated with this item. Selecting the line will set the associated property to this value.
    pub Value: String,
    /// The visible text to be assigned to the item. Optional. If this entry or the entire column is missing, the text is the same as the value.
    pub Text: String,
    /// The name of the icon to be displayed with the icon. The binary information is looked up from the Binary Table.
    pub Binary_: String,
}

impl ListView {
    #[rustfmt::skip]
    pub fn new(Property: impl Into<String>, Order: impl Into<i16>, Value: impl Into<String>, Text: impl Into<String>, Binary_: impl Into<String>) -> Self {
        Self {
            Property: Property.into(),
            Order: Order.into(),
            Value: Value.into(),
            Text: Text.into(),
            Binary_: Binary_.into(),
        }
    }
}

impl TableRow for ListView {
    const NAME: &'static str = "ListView";
    const COLUMNS: &'static str = "Property\tOrder\tValue\tText\tBinary_";
    const TYPES: &'static str = "s72\ti2\ts64\tL64\tS72";
    const META: &'static str = "ListView\tProperty\tOrder";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.Property.clone()))?;
        write!(writer, "{}\t", self.Order)?;
        write!(writer, "{}\t", escape(self.Value.clone()))?;
        write!(writer, "{}\t", escape(self.Text.clone()))?;
        write!(writer, "{}\r\n", escape(self.Binary_.clone()))?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            Property: unescape(parts[0].to_string()),
            Order: parts[1].parse().unwrap(),
            Value: unescape(parts[2].to_string()),
            Text: unescape(parts[3].to_string()),
            Binary_: unescape(parts[4].to_string()),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct LockPermissions {
    /// Foreign key into Registry or File table
    pub LockObject: String,
    /// Reference to another table name
    pub Table: String,
    /// Domain name for user whose permissions are being set. (usually a property)
    pub Domain: String,
    /// User for permissions to be set.  (usually a property)
    pub User: String,
    /// Permission Access mask.  Full Control = 268435456 (GENERIC_ALL = 0x10000000)
    pub Permission: i32,
}

impl LockPermissions {
    #[rustfmt::skip]
    pub fn new(LockObject: impl Into<String>, Table: impl Into<String>, Domain: impl Into<String>, User: impl Into<String>, Permission: impl Into<i32>) -> Self {
        Self {
            LockObject: LockObject.into(),
            Table: Table.into(),
            Domain: Domain.into(),
            User: User.into(),
            Permission: Permission.into(),
        }
    }
}

impl TableRow for LockPermissions {
    const NAME: &'static str = "LockPermissions";
    const COLUMNS: &'static str = "LockObject\tTable\tDomain\tUser\tPermission";
    const TYPES: &'static str = "s72\ts32\tS255\ts255\tI4";
    const META: &'static str = "LockPermissions\tLockObject\tTable\tDomain\tUser";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.LockObject.clone()))?;
        write!(writer, "{}\t", escape(self.Table.clone()))?;
        write!(writer, "{}\t", escape(self.Domain.clone()))?;
        write!(writer, "{}\t", escape(self.User.clone()))?;
        write!(writer, "{}\r\n", self.Permission)?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            LockObject: unescape(parts[0].to_string()),
            Table: unescape(parts[1].to_string()),
            Domain: unescape(parts[2].to_string()),
            User: unescape(parts[3].to_string()),
            Permission: parts[4].parse().unwrap(),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct Media {
    /// Primary key, integer to determine sort order for table.
    pub DiskId: i16,
    /// File sequence number for the last file for this media.
    pub LastSequence: i16,
    /// Disk name: the visible text actually printed on the disk.  This will be used to prompt the user when this disk needs to be inserted.
    pub DiskPrompt: String,
    /// If some or all of the files stored on the media are compressed in a cabinet, the name of that cabinet.
    pub Cabinet: String,
    /// The label attributed to the volume.
    pub VolumeLabel: String,
    /// The property defining the location of the cabinet file.
    pub Source: String,
}

impl Media {
    #[rustfmt::skip]
    pub fn new(DiskId: impl Into<i16>, LastSequence: impl Into<i16>, DiskPrompt: impl Into<String>, Cabinet: impl Into<String>, VolumeLabel: impl Into<String>, Source: impl Into<String>) -> Self {
        Self {
            DiskId: DiskId.into(),
            LastSequence: LastSequence.into(),
            DiskPrompt: DiskPrompt.into(),
            Cabinet: Cabinet.into(),
            VolumeLabel: VolumeLabel.into(),
            Source: Source.into(),
        }
    }
}

impl TableRow for Media {
    const NAME: &'static str = "Media";
    const COLUMNS: &'static str = "DiskId\tLastSequence\tDiskPrompt\tCabinet\tVolumeLabel\tSource";
    const TYPES: &'static str = "i2\ti2\tL64\tS255\tS32\tS72";
    const META: &'static str = "Media\tDiskId";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", self.DiskId)?;
        write!(writer, "{}\t", self.LastSequence)?;
        write!(writer, "{}\t", escape(self.DiskPrompt.clone()))?;
        write!(writer, "{}\t", escape(self.Cabinet.clone()))?;
        write!(writer, "{}\t", escape(self.VolumeLabel.clone()))?;
        write!(writer, "{}\r\n", escape(self.Source.clone()))?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            DiskId: parts[0].parse().unwrap(),
            LastSequence: parts[1].parse().unwrap(),
            DiskPrompt: unescape(parts[2].to_string()),
            Cabinet: unescape(parts[3].to_string()),
            VolumeLabel: unescape(parts[4].to_string()),
            Source: unescape(parts[5].to_string()),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct MIME {
    /// Primary key. Context identifier, typically "type/format".
    pub ContentType: String,
    /// Optional associated extension (without dot)
    pub Extension_: String,
    /// Optional associated CLSID.
    pub CLSID: String,
}

impl MIME {
    #[rustfmt::skip]
    pub fn new(ContentType: impl Into<String>, Extension_: impl Into<String>, CLSID: impl Into<String>) -> Self {
        Self {
            ContentType: ContentType.into(),
            Extension_: Extension_.into(),
            CLSID: CLSID.into(),
        }
    }
}

impl TableRow for MIME {
    const NAME: &'static str = "MIME";
    const COLUMNS: &'static str = "ContentType\tExtension_\tCLSID";
    const TYPES: &'static str = "s64\ts255\tS38";
    const META: &'static str = "MIME\tContentType";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.ContentType.clone()))?;
        write!(writer, "{}\t", escape(self.Extension_.clone()))?;
        write!(writer, "{}\r\n", escape(self.CLSID.clone()))?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            ContentType: unescape(parts[0].to_string()),
            Extension_: unescape(parts[1].to_string()),
            CLSID: unescape(parts[2].to_string()),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct ModuleComponents {
    /// Component contained in the module.
    pub Component: String,
    /// Module containing the component.
    pub ModuleID: String,
    /// Default language ID for module (may be changed by transform).
    pub Language: i16,
}

impl ModuleComponents {
    #[rustfmt::skip]
    pub fn new(Component: impl Into<String>, ModuleID: impl Into<String>, Language: impl Into<i16>) -> Self {
        Self {
            Component: Component.into(),
            ModuleID: ModuleID.into(),
            Language: Language.into(),
        }
    }
}

impl TableRow for ModuleComponents {
    const NAME: &'static str = "ModuleComponents";
    const COLUMNS: &'static str = "Component\tModuleID\tLanguage";
    const TYPES: &'static str = "s72\ts72\ti2";
    const META: &'static str = "ModuleComponents\tComponent\tModuleID\tLanguage";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.Component.clone()))?;
        write!(writer, "{}\t", escape(self.ModuleID.clone()))?;
        write!(writer, "{}\r\n", self.Language)?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            Component: unescape(parts[0].to_string()),
            ModuleID: unescape(parts[1].to_string()),
            Language: parts[2].parse().unwrap(),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct ModuleSignature {
    /// Module identifier (String.GUID).
    pub ModuleID: String,
    /// Default decimal language of module.
    pub Language: i16,
    /// Version of the module.
    pub Version: String,
}

impl ModuleSignature {
    #[rustfmt::skip]
    pub fn new(ModuleID: impl Into<String>, Language: impl Into<i16>, Version: impl Into<String>) -> Self {
        Self {
            ModuleID: ModuleID.into(),
            Language: Language.into(),
            Version: Version.into(),
        }
    }
}

impl TableRow for ModuleSignature {
    const NAME: &'static str = "ModuleSignature";
    const COLUMNS: &'static str = "ModuleID\tLanguage\tVersion";
    const TYPES: &'static str = "s72\ti2\ts32";
    const META: &'static str = "ModuleSignature\tModuleID\tLanguage";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.ModuleID.clone()))?;
        write!(writer, "{}\t", self.Language)?;
        write!(writer, "{}\r\n", escape(self.Version.clone()))?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            ModuleID: unescape(parts[0].to_string()),
            Language: parts[1].parse().unwrap(),
            Version: unescape(parts[2].to_string()),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct MoveFile {
    /// Primary key that uniquely identifies a particular MoveFile record
    pub FileKey: String,
    /// If this component is not "selected" for installation or removal, no action will be taken on the associated MoveFile entry
    pub Component_: String,
    /// Name of the source file(s) to be moved or copied.  Can contain the '*' or '?' wildcards.
    pub SourceName: String,
    /// Name to be given to the original file after it is moved or copied.  If blank, the destination file will be given the same name as the source file
    pub DestName: String,
    /// Name of a property whose value is assumed to resolve to the full path to the source directory
    pub SourceFolder: String,
    /// Name of a property whose value is assumed to resolve to the full path to the destination directory
    pub DestFolder: String,
    /// Integer value specifying the MoveFile operating mode, one of imfoEnum
    pub Options: i16,
}

impl MoveFile {
    #[rustfmt::skip]
    pub fn new(FileKey: impl Into<String>, Component_: impl Into<String>, SourceName: impl Into<String>, DestName: impl Into<String>, SourceFolder: impl Into<String>, DestFolder: impl Into<String>, Options: impl Into<i16>) -> Self {
        Self {
            FileKey: FileKey.into(),
            Component_: Component_.into(),
            SourceName: SourceName.into(),
            DestName: DestName.into(),
            SourceFolder: SourceFolder.into(),
            DestFolder: DestFolder.into(),
            Options: Options.into(),
        }
    }
}

impl TableRow for MoveFile {
    const NAME: &'static str = "MoveFile";
    const COLUMNS: &'static str = "FileKey\tComponent_\tSourceName\tDestName\tSourceFolder\tDestFolder\tOptions";
    const TYPES: &'static str = "s72\ts72\tL255\tL255\tS72\ts72\ti2";
    const META: &'static str = "MoveFile\tFileKey";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.FileKey.clone()))?;
        write!(writer, "{}\t", escape(self.Component_.clone()))?;
        write!(writer, "{}\t", escape(self.SourceName.clone()))?;
        write!(writer, "{}\t", escape(self.DestName.clone()))?;
        write!(writer, "{}\t", escape(self.SourceFolder.clone()))?;
        write!(writer, "{}\t", escape(self.DestFolder.clone()))?;
        write!(writer, "{}\r\n", self.Options)?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            FileKey: unescape(parts[0].to_string()),
            Component_: unescape(parts[1].to_string()),
            SourceName: unescape(parts[2].to_string()),
            DestName: unescape(parts[3].to_string()),
            SourceFolder: unescape(parts[4].to_string()),
            DestFolder: unescape(parts[5].to_string()),
            Options: parts[6].parse().unwrap(),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct MsiAssembly {
    /// Foreign key into Component table.
    pub Component_: String,
    /// Foreign key into Feature table.
    pub Feature_: String,
    /// Foreign key into the File table denoting the manifest file for the assembly.
    pub File_Manifest: String,
    /// Foreign key into File table, denoting the application context for private assemblies. Null for global assemblies.
    pub File_Application: String,
    /// Assembly attributes
    pub Attributes: i16,
}

impl MsiAssembly {
    #[rustfmt::skip]
    pub fn new(Component_: impl Into<String>, Feature_: impl Into<String>, File_Manifest: impl Into<String>, File_Application: impl Into<String>, Attributes: impl Into<i16>) -> Self {
        Self {
            Component_: Component_.into(),
            Feature_: Feature_.into(),
            File_Manifest: File_Manifest.into(),
            File_Application: File_Application.into(),
            Attributes: Attributes.into(),
        }
    }
}

impl TableRow for MsiAssembly {
    const NAME: &'static str = "MsiAssembly";
    const COLUMNS: &'static str = "Component_\tFeature_\tFile_Manifest\tFile_Application\tAttributes";
    const TYPES: &'static str = "s72\ts38\tS72\tS72\tI2";
    const META: &'static str = "MsiAssembly\tComponent_";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.Component_.clone()))?;
        write!(writer, "{}\t", escape(self.Feature_.clone()))?;
        write!(writer, "{}\t", escape(self.File_Manifest.clone()))?;
        write!(writer, "{}\t", escape(self.File_Application.clone()))?;
        write!(writer, "{}\r\n", self.Attributes)?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            Component_: unescape(parts[0].to_string()),
            Feature_: unescape(parts[1].to_string()),
            File_Manifest: unescape(parts[2].to_string()),
            File_Application: unescape(parts[3].to_string()),
            Attributes: parts[4].parse().unwrap(),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct MsiAssemblyName {
    /// Foreign key into Component table.
    pub Component_: String,
    /// The name part of the name-value pairs for the assembly name.
    pub Name: String,
    /// The value part of the name-value pairs for the assembly name.
    pub Value: String,
}

impl MsiAssemblyName {
    #[rustfmt::skip]
    pub fn new(Component_: impl Into<String>, Name: impl Into<String>, Value: impl Into<String>) -> Self {
        Self {
            Component_: Component_.into(),
            Name: Name.into(),
            Value: Value.into(),
        }
    }
}

impl TableRow for MsiAssemblyName {
    const NAME: &'static str = "MsiAssemblyName";
    const COLUMNS: &'static str = "Component_\tName\tValue";
    const TYPES: &'static str = "s72\ts255\ts255";
    const META: &'static str = "MsiAssemblyName\tComponent_\tName";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.Component_.clone()))?;
        write!(writer, "{}\t", escape(self.Name.clone()))?;
        write!(writer, "{}\r\n", escape(self.Value.clone()))?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            Component_: unescape(parts[0].to_string()),
            Name: unescape(parts[1].to_string()),
            Value: unescape(parts[2].to_string()),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct MsiFileHash {
    /// Primary key, foreign key into File table referencing file with this hash
    pub File_: String,
    /// Various options and attributes for this hash.
    pub Options: i16,
    /// Size of file in bytes (long integer).
    pub HashPart1: i32,
    /// Size of file in bytes (long integer).
    pub HashPart2: i32,
    /// Size of file in bytes (long integer).
    pub HashPart3: i32,
    /// Size of file in bytes (long integer).
    pub HashPart4: i32,
}

impl MsiFileHash {
    #[rustfmt::skip]
    pub fn new(File_: impl Into<String>, Options: impl Into<i16>, HashPart1: impl Into<i32>, HashPart2: impl Into<i32>, HashPart3: impl Into<i32>, HashPart4: impl Into<i32>) -> Self {
        Self {
            File_: File_.into(),
            Options: Options.into(),
            HashPart1: HashPart1.into(),
            HashPart2: HashPart2.into(),
            HashPart3: HashPart3.into(),
            HashPart4: HashPart4.into(),
        }
    }
}

impl TableRow for MsiFileHash {
    const NAME: &'static str = "MsiFileHash";
    const COLUMNS: &'static str = "File_\tOptions\tHashPart1\tHashPart2\tHashPart3\tHashPart4";
    const TYPES: &'static str = "s72\ti2\ti4\ti4\ti4\ti4";
    const META: &'static str = "MsiFileHash\tFile_";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.File_.clone()))?;
        write!(writer, "{}\t", self.Options)?;
        write!(writer, "{}\t", self.HashPart1)?;
        write!(writer, "{}\t", self.HashPart2)?;
        write!(writer, "{}\t", self.HashPart3)?;
        write!(writer, "{}\r\n", self.HashPart4)?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            File_: unescape(parts[0].to_string()),
            Options: parts[1].parse().unwrap(),
            HashPart1: parts[2].parse().unwrap(),
            HashPart2: parts[3].parse().unwrap(),
            HashPart3: parts[4].parse().unwrap(),
            HashPart4: parts[5].parse().unwrap(),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct ODBCAttribute {
    /// Reference to ODBC driver in ODBCDriver table
    pub Driver_: String,
    /// Name of ODBC driver attribute
    pub Attribute: String,
    /// Value for ODBC driver attribute
    pub Value: String,
}

impl ODBCAttribute {
    #[rustfmt::skip]
    pub fn new(Driver_: impl Into<String>, Attribute: impl Into<String>, Value: impl Into<String>) -> Self {
        Self {
            Driver_: Driver_.into(),
            Attribute: Attribute.into(),
            Value: Value.into(),
        }
    }
}

impl TableRow for ODBCAttribute {
    const NAME: &'static str = "ODBCAttribute";
    const COLUMNS: &'static str = "Driver_\tAttribute\tValue";
    const TYPES: &'static str = "s72\ts40\tL255";
    const META: &'static str = "ODBCAttribute\tDriver_\tAttribute";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.Driver_.clone()))?;
        write!(writer, "{}\t", escape(self.Attribute.clone()))?;
        write!(writer, "{}\r\n", escape(self.Value.clone()))?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            Driver_: unescape(parts[0].to_string()),
            Attribute: unescape(parts[1].to_string()),
            Value: unescape(parts[2].to_string()),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct ODBCDataSource {
    /// Primary key, non-localized.internal token for data source
    pub DataSource: String,
    /// Reference to associated component
    pub Component_: String,
    /// Text used as registered name for data source
    pub Description: String,
    /// Reference to driver description, may be existing driver
    pub DriverDescription: String,
    /// Registration option: 0=machine, 1=user, others t.b.d.
    pub Registration: i16,
}

impl ODBCDataSource {
    #[rustfmt::skip]
    pub fn new(DataSource: impl Into<String>, Component_: impl Into<String>, Description: impl Into<String>, DriverDescription: impl Into<String>, Registration: impl Into<i16>) -> Self {
        Self {
            DataSource: DataSource.into(),
            Component_: Component_.into(),
            Description: Description.into(),
            DriverDescription: DriverDescription.into(),
            Registration: Registration.into(),
        }
    }
}

impl TableRow for ODBCDataSource {
    const NAME: &'static str = "ODBCDataSource";
    const COLUMNS: &'static str = "DataSource\tComponent_\tDescription\tDriverDescription\tRegistration";
    const TYPES: &'static str = "s72\ts72\ts255\ts255\ti2";
    const META: &'static str = "ODBCDataSource\tDataSource";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.DataSource.clone()))?;
        write!(writer, "{}\t", escape(self.Component_.clone()))?;
        write!(writer, "{}\t", escape(self.Description.clone()))?;
        write!(writer, "{}\t", escape(self.DriverDescription.clone()))?;
        write!(writer, "{}\r\n", self.Registration)?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            DataSource: unescape(parts[0].to_string()),
            Component_: unescape(parts[1].to_string()),
            Description: unescape(parts[2].to_string()),
            DriverDescription: unescape(parts[3].to_string()),
            Registration: parts[4].parse().unwrap(),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct ODBCDriver {
    /// Primary key, non-localized.internal token for driver
    pub Driver: String,
    /// Reference to associated component
    pub Component_: String,
    /// Text used as registered name for driver, non-localized
    pub Description: String,
    /// Reference to key driver file
    pub File_: String,
    /// Optional reference to key driver setup DLL
    pub File_Setup: String,
}

impl ODBCDriver {
    #[rustfmt::skip]
    pub fn new(Driver: impl Into<String>, Component_: impl Into<String>, Description: impl Into<String>, File_: impl Into<String>, File_Setup: impl Into<String>) -> Self {
        Self {
            Driver: Driver.into(),
            Component_: Component_.into(),
            Description: Description.into(),
            File_: File_.into(),
            File_Setup: File_Setup.into(),
        }
    }
}

impl TableRow for ODBCDriver {
    const NAME: &'static str = "ODBCDriver";
    const COLUMNS: &'static str = "Driver\tComponent_\tDescription\tFile_\tFile_Setup";
    const TYPES: &'static str = "s72\ts72\ts255\ts72\tS72";
    const META: &'static str = "ODBCDriver\tDriver";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.Driver.clone()))?;
        write!(writer, "{}\t", escape(self.Component_.clone()))?;
        write!(writer, "{}\t", escape(self.Description.clone()))?;
        write!(writer, "{}\t", escape(self.File_.clone()))?;
        write!(writer, "{}\r\n", escape(self.File_Setup.clone()))?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            Driver: unescape(parts[0].to_string()),
            Component_: unescape(parts[1].to_string()),
            Description: unescape(parts[2].to_string()),
            File_: unescape(parts[3].to_string()),
            File_Setup: unescape(parts[4].to_string()),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct ODBCSourceAttribute {
    /// Reference to ODBC data source in ODBCDataSource table
    pub DataSource_: String,
    /// Name of ODBC data source attribute
    pub Attribute: String,
    /// Value for ODBC data source attribute
    pub Value: String,
}

impl ODBCSourceAttribute {
    #[rustfmt::skip]
    pub fn new(DataSource_: impl Into<String>, Attribute: impl Into<String>, Value: impl Into<String>) -> Self {
        Self {
            DataSource_: DataSource_.into(),
            Attribute: Attribute.into(),
            Value: Value.into(),
        }
    }
}

impl TableRow for ODBCSourceAttribute {
    const NAME: &'static str = "ODBCSourceAttribute";
    const COLUMNS: &'static str = "DataSource_\tAttribute\tValue";
    const TYPES: &'static str = "s72\ts32\tL255";
    const META: &'static str = "ODBCSourceAttribute\tDataSource_\tAttribute";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.DataSource_.clone()))?;
        write!(writer, "{}\t", escape(self.Attribute.clone()))?;
        write!(writer, "{}\r\n", escape(self.Value.clone()))?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            DataSource_: unescape(parts[0].to_string()),
            Attribute: unescape(parts[1].to_string()),
            Value: unescape(parts[2].to_string()),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct ODBCTranslator {
    /// Primary key, non-localized.internal token for translator
    pub Translator: String,
    /// Reference to associated component
    pub Component_: String,
    /// Text used as registered name for translator
    pub Description: String,
    /// Reference to key translator file
    pub File_: String,
    /// Optional reference to key translator setup DLL
    pub File_Setup: String,
}

impl ODBCTranslator {
    #[rustfmt::skip]
    pub fn new(Translator: impl Into<String>, Component_: impl Into<String>, Description: impl Into<String>, File_: impl Into<String>, File_Setup: impl Into<String>) -> Self {
        Self {
            Translator: Translator.into(),
            Component_: Component_.into(),
            Description: Description.into(),
            File_: File_.into(),
            File_Setup: File_Setup.into(),
        }
    }
}

impl TableRow for ODBCTranslator {
    const NAME: &'static str = "ODBCTranslator";
    const COLUMNS: &'static str = "Translator\tComponent_\tDescription\tFile_\tFile_Setup";
    const TYPES: &'static str = "s72\ts72\ts255\ts72\tS72";
    const META: &'static str = "ODBCTranslator\tTranslator";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.Translator.clone()))?;
        write!(writer, "{}\t", escape(self.Component_.clone()))?;
        write!(writer, "{}\t", escape(self.Description.clone()))?;
        write!(writer, "{}\t", escape(self.File_.clone()))?;
        write!(writer, "{}\r\n", escape(self.File_Setup.clone()))?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            Translator: unescape(parts[0].to_string()),
            Component_: unescape(parts[1].to_string()),
            Description: unescape(parts[2].to_string()),
            File_: unescape(parts[3].to_string()),
            File_Setup: unescape(parts[4].to_string()),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct PatchPackage {
    /// A unique string GUID representing this patch.
    pub PatchId: String,
    /// Foreign key to DiskId column of Media table. Indicates the disk containing the patch package.
    pub Media_: i16,
}

impl PatchPackage {
    #[rustfmt::skip]
    pub fn new(PatchId: impl Into<String>, Media_: impl Into<i16>) -> Self {
        Self {
            PatchId: PatchId.into(),
            Media_: Media_.into(),
        }
    }
}

impl TableRow for PatchPackage {
    const NAME: &'static str = "PatchPackage";
    const COLUMNS: &'static str = "PatchId\tMedia_";
    const TYPES: &'static str = "s38\ti2";
    const META: &'static str = "PatchPackage\tPatchId";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.PatchId.clone()))?;
        write!(writer, "{}\r\n", self.Media_)?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            PatchId: unescape(parts[0].to_string()),
            Media_: parts[1].parse().unwrap(),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct ProgId {
    /// The Program Identifier. Primary key.
    pub ProgId: String,
    /// The Parent Program Identifier. If specified, the ProgId column becomes a version independent prog id.
    pub ProgId_Parent: String,
    /// The CLSID of an OLE factory corresponding to the ProgId.
    pub Class_: String,
    /// Localized description for the Program identifier.
    pub Description: String,
    /// Optional foreign key into the Icon Table, specifying the icon file associated with this ProgId. Will be written under the DefaultIcon key.
    pub Icon_: String,
    /// Optional icon index.
    pub IconIndex: i16,
}

impl ProgId {
    #[rustfmt::skip]
    pub fn new(ProgId: impl Into<String>, ProgId_Parent: impl Into<String>, Class_: impl Into<String>, Description: impl Into<String>, Icon_: impl Into<String>, IconIndex: impl Into<i16>) -> Self {
        Self {
            ProgId: ProgId.into(),
            ProgId_Parent: ProgId_Parent.into(),
            Class_: Class_.into(),
            Description: Description.into(),
            Icon_: Icon_.into(),
            IconIndex: IconIndex.into(),
        }
    }
}

impl TableRow for ProgId {
    const NAME: &'static str = "ProgId";
    const COLUMNS: &'static str = "ProgId\tProgId_Parent\tClass_\tDescription\tIcon_\tIconIndex";
    const TYPES: &'static str = "s255\tS255\tS38\tL255\tS72\tI2";
    const META: &'static str = "ProgId\tProgId";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.ProgId.clone()))?;
        write!(writer, "{}\t", escape(self.ProgId_Parent.clone()))?;
        write!(writer, "{}\t", escape(self.Class_.clone()))?;
        write!(writer, "{}\t", escape(self.Description.clone()))?;
        write!(writer, "{}\t", escape(self.Icon_.clone()))?;
        write!(writer, "{}\r\n", self.IconIndex)?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            ProgId: unescape(parts[0].to_string()),
            ProgId_Parent: unescape(parts[1].to_string()),
            Class_: unescape(parts[2].to_string()),
            Description: unescape(parts[3].to_string()),
            Icon_: unescape(parts[4].to_string()),
            IconIndex: parts[5].parse().unwrap(),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct Property {
    /// Name of property, uppercase if settable by launcher or loader.
    pub Property: String,
    /// String value for property.  Never null or empty.
    pub Value: String,
}

impl Property {
    #[rustfmt::skip]
    pub fn new(Property: impl Into<String>, Value: impl Into<String>) -> Self {
        Self {
            Property: Property.into(),
            Value: Value.into(),
        }
    }
}

impl TableRow for Property {
    const NAME: &'static str = "Property";
    const COLUMNS: &'static str = "Property\tValue";
    const TYPES: &'static str = "s72\tl0";
    const META: &'static str = "Property\tProperty";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.Property.clone()))?;
        write!(writer, "{}\r\n", escape(self.Value.clone()))?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            Property: unescape(parts[0].to_string()),
            Value: unescape(parts[1].to_string()),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct PublishComponent {
    /// A string GUID that represents the component id that will be requested by the alien product.
    pub ComponentId: String,
    /// This is defined only when the ComponentId column is an Qualified Component Id. This is the Qualifier for ProvideComponentIndirect.
    pub Qualifier: String,
    /// Foreign key into the Component table.
    pub Component_: String,
    /// This is localisable Application specific data that can be associated with a Qualified Component.
    pub AppData: String,
    /// Foreign key into the Feature table.
    pub Feature_: String,
}

impl PublishComponent {
    #[rustfmt::skip]
    pub fn new(ComponentId: impl Into<String>, Qualifier: impl Into<String>, Component_: impl Into<String>, AppData: impl Into<String>, Feature_: impl Into<String>) -> Self {
        Self {
            ComponentId: ComponentId.into(),
            Qualifier: Qualifier.into(),
            Component_: Component_.into(),
            AppData: AppData.into(),
            Feature_: Feature_.into(),
        }
    }
}

impl TableRow for PublishComponent {
    const NAME: &'static str = "PublishComponent";
    const COLUMNS: &'static str = "ComponentId\tQualifier\tComponent_\tAppData\tFeature_";
    const TYPES: &'static str = "s38\ts255\ts72\tL255\ts38";
    const META: &'static str = "PublishComponent\tComponentId\tQualifier\tComponent_";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.ComponentId.clone()))?;
        write!(writer, "{}\t", escape(self.Qualifier.clone()))?;
        write!(writer, "{}\t", escape(self.Component_.clone()))?;
        write!(writer, "{}\t", escape(self.AppData.clone()))?;
        write!(writer, "{}\r\n", escape(self.Feature_.clone()))?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            ComponentId: unescape(parts[0].to_string()),
            Qualifier: unescape(parts[1].to_string()),
            Component_: unescape(parts[2].to_string()),
            AppData: unescape(parts[3].to_string()),
            Feature_: unescape(parts[4].to_string()),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct RadioButton {
    /// A named property to be tied to this radio button. All the buttons tied to the same property become part of the same group.
    pub Property: String,
    /// A positive integer used to determine the ordering of the items within one list..The integers do not have to be consecutive.
    pub Order: i16,
    /// The value string associated with this button. Selecting the button will set the associated property to this value.
    pub Value: String,
    /// The horizontal coordinate of the upper left corner of the bounding rectangle of the radio button.
    pub X: i16,
    /// The vertical coordinate of the upper left corner of the bounding rectangle of the radio button.
    pub Y: i16,
    /// The width of the button.
    pub Width: i16,
    /// The height of the button.
    pub Height: i16,
    /// The visible title to be assigned to the radio button.
    pub Text: String,
    /// The help strings used with the button. The text is optional.
    pub Help: String,
}

impl RadioButton {
    #[rustfmt::skip]
    pub fn new(Property: impl Into<String>, Order: impl Into<i16>, Value: impl Into<String>, X: impl Into<i16>, Y: impl Into<i16>, Width: impl Into<i16>, Height: impl Into<i16>, Text: impl Into<String>, Help: impl Into<String>) -> Self {
        Self {
            Property: Property.into(),
            Order: Order.into(),
            Value: Value.into(),
            X: X.into(),
            Y: Y.into(),
            Width: Width.into(),
            Height: Height.into(),
            Text: Text.into(),
            Help: Help.into(),
        }
    }
}

impl TableRow for RadioButton {
    const NAME: &'static str = "RadioButton";
    const COLUMNS: &'static str = "Property\tOrder\tValue\tX\tY\tWidth\tHeight\tText\tHelp";
    const TYPES: &'static str = "s72\ti2\ts64\ti2\ti2\ti2\ti2\tL64\tL50";
    const META: &'static str = "RadioButton\tProperty\tOrder";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.Property.clone()))?;
        write!(writer, "{}\t", self.Order)?;
        write!(writer, "{}\t", escape(self.Value.clone()))?;
        write!(writer, "{}\t", self.X)?;
        write!(writer, "{}\t", self.Y)?;
        write!(writer, "{}\t", self.Width)?;
        write!(writer, "{}\t", self.Height)?;
        write!(writer, "{}\t", escape(self.Text.clone()))?;
        write!(writer, "{}\r\n", escape(self.Help.clone()))?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            Property: unescape(parts[0].to_string()),
            Order: parts[1].parse().unwrap(),
            Value: unescape(parts[2].to_string()),
            X: parts[3].parse().unwrap(),
            Y: parts[4].parse().unwrap(),
            Width: parts[5].parse().unwrap(),
            Height: parts[6].parse().unwrap(),
            Text: unescape(parts[7].to_string()),
            Help: unescape(parts[8].to_string()),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct Registry {
    /// Primary key, non-localized token.
    pub Registry: String,
    /// The predefined root key for the registry value, one of rrkEnum.
    pub Root: i16,
    /// The key for the registry value.
    pub Key: String,
    /// The registry value name.
    pub Name: String,
    /// The registry value.
    pub Value: String,
    /// Foreign key into the Component table referencing component that controls the installing of the registry value.
    pub Component_: String,
}

impl Registry {
    #[rustfmt::skip]
    pub fn new(Registry: impl Into<String>, Root: impl Into<i16>, Key: impl Into<String>, Name: impl Into<String>, Value: impl Into<String>, Component_: impl Into<String>) -> Self {
        Self {
            Registry: Registry.into(),
            Root: Root.into(),
            Key: Key.into(),
            Name: Name.into(),
            Value: Value.into(),
            Component_: Component_.into(),
        }
    }
}

impl TableRow for Registry {
    const NAME: &'static str = "Registry";
    const COLUMNS: &'static str = "Registry\tRoot\tKey\tName\tValue\tComponent_";
    const TYPES: &'static str = "s72\ti2\tl255\tL255\tL0\ts72";
    const META: &'static str = "Registry\tRegistry";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.Registry.clone()))?;
        write!(writer, "{}\t", self.Root)?;
        write!(writer, "{}\t", escape(self.Key.clone()))?;
        write!(writer, "{}\t", escape(self.Name.clone()))?;
        write!(writer, "{}\t", escape(self.Value.clone()))?;
        write!(writer, "{}\r\n", escape(self.Component_.clone()))?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            Registry: unescape(parts[0].to_string()),
            Root: parts[1].parse().unwrap(),
            Key: unescape(parts[2].to_string()),
            Name: unescape(parts[3].to_string()),
            Value: unescape(parts[4].to_string()),
            Component_: unescape(parts[5].to_string()),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct RegLocator {
    /// The table key. The Signature_ represents a unique file signature and is also the foreign key in the Signature table. If the type is 0, the registry values refers a directory, and _Signature is not a foreign key.
    pub Signature_: String,
    /// The predefined root key for the registry value, one of rrkEnum.
    pub Root: i16,
    /// The key for the registry value.
    pub Key: String,
    /// The registry value name.
    pub Name: String,
    /// An integer value that determines if the registry value is a filename or a directory location or to be used as is w/o interpretation.
    pub Type: i16,
}

impl RegLocator {
    #[rustfmt::skip]
    pub fn new(Signature_: impl Into<String>, Root: impl Into<i16>, Key: impl Into<String>, Name: impl Into<String>, Type: impl Into<i16>) -> Self {
        Self {
            Signature_: Signature_.into(),
            Root: Root.into(),
            Key: Key.into(),
            Name: Name.into(),
            Type: Type.into(),
        }
    }
}

impl TableRow for RegLocator {
    const NAME: &'static str = "RegLocator";
    const COLUMNS: &'static str = "Signature_\tRoot\tKey\tName\tType";
    const TYPES: &'static str = "s72\ti2\ts255\tS255\tI2";
    const META: &'static str = "RegLocator\tSignature_";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.Signature_.clone()))?;
        write!(writer, "{}\t", self.Root)?;
        write!(writer, "{}\t", escape(self.Key.clone()))?;
        write!(writer, "{}\t", escape(self.Name.clone()))?;
        write!(writer, "{}\r\n", self.Type)?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            Signature_: unescape(parts[0].to_string()),
            Root: parts[1].parse().unwrap(),
            Key: unescape(parts[2].to_string()),
            Name: unescape(parts[3].to_string()),
            Type: parts[4].parse().unwrap(),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct RemoveFile {
    /// Primary key used to identify a particular file entry
    pub FileKey: String,
    /// Foreign key referencing Component that controls the file to be removed.
    pub Component_: String,
    /// Name of the file to be removed.
    pub FileName: String,
    /// Name of a property whose value is assumed to resolve to the full pathname to the folder of the file to be removed.
    pub DirProperty: String,
    /// Installation option, one of iimEnum.
    pub InstallMode: i16,
}

impl RemoveFile {
    #[rustfmt::skip]
    pub fn new(FileKey: impl Into<String>, Component_: impl Into<String>, FileName: impl Into<String>, DirProperty: impl Into<String>, InstallMode: impl Into<i16>) -> Self {
        Self {
            FileKey: FileKey.into(),
            Component_: Component_.into(),
            FileName: FileName.into(),
            DirProperty: DirProperty.into(),
            InstallMode: InstallMode.into(),
        }
    }
}

impl TableRow for RemoveFile {
    const NAME: &'static str = "RemoveFile";
    const COLUMNS: &'static str = "FileKey\tComponent_\tFileName\tDirProperty\tInstallMode";
    const TYPES: &'static str = "s72\ts72\tL255\ts72\ti2";
    const META: &'static str = "RemoveFile\tFileKey";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.FileKey.clone()))?;
        write!(writer, "{}\t", escape(self.Component_.clone()))?;
        write!(writer, "{}\t", escape(self.FileName.clone()))?;
        write!(writer, "{}\t", escape(self.DirProperty.clone()))?;
        write!(writer, "{}\r\n", self.InstallMode)?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            FileKey: unescape(parts[0].to_string()),
            Component_: unescape(parts[1].to_string()),
            FileName: unescape(parts[2].to_string()),
            DirProperty: unescape(parts[3].to_string()),
            InstallMode: parts[4].parse().unwrap(),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct RemoveIniFile {
    /// Primary key, non-localized token.
    pub RemoveIniFile: String,
    /// The .INI file name in which to delete the information
    pub FileName: String,
    /// Foreign key into the Directory table denoting the directory where the .INI file is.
    pub DirProperty: String,
    /// The .INI file Section.
    pub Section: String,
    /// The .INI file key below Section.
    pub Key: String,
    /// The value to be deleted. The value is required when Action is iifIniRemoveTag
    pub Value: String,
    /// The type of modification to be made, one of iifEnum.
    pub Action: i16,
    /// Foreign key into the Component table referencing component that controls the deletion of the .INI value.
    pub Component_: String,
}

impl RemoveIniFile {
    #[rustfmt::skip]
    pub fn new(RemoveIniFile: impl Into<String>, FileName: impl Into<String>, DirProperty: impl Into<String>, Section: impl Into<String>, Key: impl Into<String>, Value: impl Into<String>, Action: impl Into<i16>, Component_: impl Into<String>) -> Self {
        Self {
            RemoveIniFile: RemoveIniFile.into(),
            FileName: FileName.into(),
            DirProperty: DirProperty.into(),
            Section: Section.into(),
            Key: Key.into(),
            Value: Value.into(),
            Action: Action.into(),
            Component_: Component_.into(),
        }
    }
}

impl TableRow for RemoveIniFile {
    const NAME: &'static str = "RemoveIniFile";
    const COLUMNS: &'static str = "RemoveIniFile\tFileName\tDirProperty\tSection\tKey\tValue\tAction\tComponent_";
    const TYPES: &'static str = "s72\tl255\tS72\tl96\tl128\tL255\ti2\ts72";
    const META: &'static str = "RemoveIniFile\tRemoveIniFile";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.RemoveIniFile.clone()))?;
        write!(writer, "{}\t", escape(self.FileName.clone()))?;
        write!(writer, "{}\t", escape(self.DirProperty.clone()))?;
        write!(writer, "{}\t", escape(self.Section.clone()))?;
        write!(writer, "{}\t", escape(self.Key.clone()))?;
        write!(writer, "{}\t", escape(self.Value.clone()))?;
        write!(writer, "{}\t", self.Action)?;
        write!(writer, "{}\r\n", escape(self.Component_.clone()))?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            RemoveIniFile: unescape(parts[0].to_string()),
            FileName: unescape(parts[1].to_string()),
            DirProperty: unescape(parts[2].to_string()),
            Section: unescape(parts[3].to_string()),
            Key: unescape(parts[4].to_string()),
            Value: unescape(parts[5].to_string()),
            Action: parts[6].parse().unwrap(),
            Component_: unescape(parts[7].to_string()),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct RemoveRegistry {
    /// Primary key, non-localized token.
    pub RemoveRegistry: String,
    /// The predefined root key for the registry value, one of rrkEnum
    pub Root: i16,
    /// The key for the registry value.
    pub Key: String,
    /// The registry value name.
    pub Name: String,
    /// Foreign key into the Component table referencing component that controls the deletion of the registry value.
    pub Component_: String,
}

impl RemoveRegistry {
    #[rustfmt::skip]
    pub fn new(RemoveRegistry: impl Into<String>, Root: impl Into<i16>, Key: impl Into<String>, Name: impl Into<String>, Component_: impl Into<String>) -> Self {
        Self {
            RemoveRegistry: RemoveRegistry.into(),
            Root: Root.into(),
            Key: Key.into(),
            Name: Name.into(),
            Component_: Component_.into(),
        }
    }
}

impl TableRow for RemoveRegistry {
    const NAME: &'static str = "RemoveRegistry";
    const COLUMNS: &'static str = "RemoveRegistry\tRoot\tKey\tName\tComponent_";
    const TYPES: &'static str = "s72\ti2\tl255\tL255\ts72";
    const META: &'static str = "RemoveRegistry\tRemoveRegistry";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.RemoveRegistry.clone()))?;
        write!(writer, "{}\t", self.Root)?;
        write!(writer, "{}\t", escape(self.Key.clone()))?;
        write!(writer, "{}\t", escape(self.Name.clone()))?;
        write!(writer, "{}\r\n", escape(self.Component_.clone()))?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            RemoveRegistry: unescape(parts[0].to_string()),
            Root: parts[1].parse().unwrap(),
            Key: unescape(parts[2].to_string()),
            Name: unescape(parts[3].to_string()),
            Component_: unescape(parts[4].to_string()),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct ReserveCost {
    /// Primary key that uniquely identifies a particular ReserveCost record
    pub ReserveKey: String,
    /// Reserve a specified amount of space if this component is to be installed.
    pub Component_: String,
    /// Name of a property whose value is assumed to resolve to the full path to the destination directory
    pub ReserveFolder: String,
    /// Disk space to reserve if linked component is installed locally.
    pub ReserveLocal: i32,
    /// Disk space to reserve if linked component is installed to run from the source location.
    pub ReserveSource: i32,
}

impl ReserveCost {
    #[rustfmt::skip]
    pub fn new(ReserveKey: impl Into<String>, Component_: impl Into<String>, ReserveFolder: impl Into<String>, ReserveLocal: impl Into<i32>, ReserveSource: impl Into<i32>) -> Self {
        Self {
            ReserveKey: ReserveKey.into(),
            Component_: Component_.into(),
            ReserveFolder: ReserveFolder.into(),
            ReserveLocal: ReserveLocal.into(),
            ReserveSource: ReserveSource.into(),
        }
    }
}

impl TableRow for ReserveCost {
    const NAME: &'static str = "ReserveCost";
    const COLUMNS: &'static str = "ReserveKey\tComponent_\tReserveFolder\tReserveLocal\tReserveSource";
    const TYPES: &'static str = "s72\ts72\tS72\ti4\ti4";
    const META: &'static str = "ReserveCost\tReserveKey";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.ReserveKey.clone()))?;
        write!(writer, "{}\t", escape(self.Component_.clone()))?;
        write!(writer, "{}\t", escape(self.ReserveFolder.clone()))?;
        write!(writer, "{}\t", self.ReserveLocal)?;
        write!(writer, "{}\r\n", self.ReserveSource)?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            ReserveKey: unescape(parts[0].to_string()),
            Component_: unescape(parts[1].to_string()),
            ReserveFolder: unescape(parts[2].to_string()),
            ReserveLocal: parts[3].parse().unwrap(),
            ReserveSource: parts[4].parse().unwrap(),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct SelfReg {
    /// Foreign key into the File table denoting the module that needs to be registered.
    pub File_: String,
    /// The cost of registering the module.
    pub Cost: i16,
}

impl SelfReg {
    #[rustfmt::skip]
    pub fn new(File_: impl Into<String>, Cost: impl Into<i16>) -> Self {
        Self {
            File_: File_.into(),
            Cost: Cost.into(),
        }
    }
}

impl TableRow for SelfReg {
    const NAME: &'static str = "SelfReg";
    const COLUMNS: &'static str = "File_\tCost";
    const TYPES: &'static str = "s72\tI2";
    const META: &'static str = "SelfReg\tFile_";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.File_.clone()))?;
        write!(writer, "{}\r\n", self.Cost)?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            File_: unescape(parts[0].to_string()),
            Cost: parts[1].parse().unwrap(),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct ServiceControl {
    /// Primary key, non-localized token.
    pub ServiceControl: String,
    /// Name of a service. /, \, comma and space are invalid
    pub Name: String,
    /// Bit field:  Install:  0x1 = Start, 0x2 = Stop, 0x8 = Delete, Uninstall: 0x10 = Start, 0x20 = Stop, 0x80 = Delete
    pub Event: i16,
    /// Arguments for the service.  Separate by [~].
    pub Arguments: String,
    /// Boolean for whether to wait for the service to fully start
    pub Wait: i16,
    /// Required foreign key into the Component Table that controls the startup of the service
    pub Component_: String,
}

impl ServiceControl {
    #[rustfmt::skip]
    pub fn new(ServiceControl: impl Into<String>, Name: impl Into<String>, Event: impl Into<i16>, Arguments: impl Into<String>, Wait: impl Into<i16>, Component_: impl Into<String>) -> Self {
        Self {
            ServiceControl: ServiceControl.into(),
            Name: Name.into(),
            Event: Event.into(),
            Arguments: Arguments.into(),
            Wait: Wait.into(),
            Component_: Component_.into(),
        }
    }
}

impl TableRow for ServiceControl {
    const NAME: &'static str = "ServiceControl";
    const COLUMNS: &'static str = "ServiceControl\tName\tEvent\tArguments\tWait\tComponent_";
    const TYPES: &'static str = "s72\tl255\ti2\tL255\tI2\ts72";
    const META: &'static str = "ServiceControl\tServiceControl";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.ServiceControl.clone()))?;
        write!(writer, "{}\t", escape(self.Name.clone()))?;
        write!(writer, "{}\t", self.Event)?;
        write!(writer, "{}\t", escape(self.Arguments.clone()))?;
        write!(writer, "{}\t", self.Wait)?;
        write!(writer, "{}\r\n", escape(self.Component_.clone()))?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            ServiceControl: unescape(parts[0].to_string()),
            Name: unescape(parts[1].to_string()),
            Event: parts[2].parse().unwrap(),
            Arguments: unescape(parts[3].to_string()),
            Wait: parts[4].parse().unwrap(),
            Component_: unescape(parts[5].to_string()),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct ServiceInstall {
    /// Primary key, non-localized token.
    pub ServiceInstall: String,
    /// Internal Name of the Service
    pub Name: String,
    /// External Name of the Service
    pub DisplayName: String,
    /// Type of the service
    pub ServiceType: i32,
    /// Type of the service
    pub StartType: i32,
    /// Severity of error if service fails to start
    pub ErrorControl: i32,
    /// LoadOrderGroup
    pub LoadOrderGroup: String,
    /// Other services this depends on to start.  Separate by [~], and end with [~][~]
    pub Dependencies: String,
    /// User or object name to run service as
    pub StartName: String,
    /// password to run service with.  (with StartName)
    pub Password: String,
    /// Arguments to include in every start of the service, passed to WinMain
    pub Arguments: String,
    /// Required foreign key into the Component Table that controls the startup of the service
    pub Component_: String,
    /// Description of service.
    pub Description: String,
}

impl ServiceInstall {
    #[rustfmt::skip]
    pub fn new(ServiceInstall: impl Into<String>, Name: impl Into<String>, DisplayName: impl Into<String>, ServiceType: impl Into<i32>, StartType: impl Into<i32>, ErrorControl: impl Into<i32>, LoadOrderGroup: impl Into<String>, Dependencies: impl Into<String>, StartName: impl Into<String>, Password: impl Into<String>, Arguments: impl Into<String>, Component_: impl Into<String>, Description: impl Into<String>) -> Self {
        Self {
            ServiceInstall: ServiceInstall.into(),
            Name: Name.into(),
            DisplayName: DisplayName.into(),
            ServiceType: ServiceType.into(),
            StartType: StartType.into(),
            ErrorControl: ErrorControl.into(),
            LoadOrderGroup: LoadOrderGroup.into(),
            Dependencies: Dependencies.into(),
            StartName: StartName.into(),
            Password: Password.into(),
            Arguments: Arguments.into(),
            Component_: Component_.into(),
            Description: Description.into(),
        }
    }
}

impl TableRow for ServiceInstall {
    const NAME: &'static str = "ServiceInstall";
    const COLUMNS: &'static str = "ServiceInstall\tName\tDisplayName\tServiceType\tStartType\tErrorControl\tLoadOrderGroup\tDependencies\tStartName\tPassword\tArguments\tComponent_\tDescription";
    const TYPES: &'static str = "s72\ts255\tL255\ti4\ti4\ti4\tS255\tS255\tS255\tS255\tS255\ts72\tL255";
    const META: &'static str = "ServiceInstall\tServiceInstall";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.ServiceInstall.clone()))?;
        write!(writer, "{}\t", escape(self.Name.clone()))?;
        write!(writer, "{}\t", escape(self.DisplayName.clone()))?;
        write!(writer, "{}\t", self.ServiceType)?;
        write!(writer, "{}\t", self.StartType)?;
        write!(writer, "{}\t", self.ErrorControl)?;
        write!(writer, "{}\t", escape(self.LoadOrderGroup.clone()))?;
        write!(writer, "{}\t", escape(self.Dependencies.clone()))?;
        write!(writer, "{}\t", escape(self.StartName.clone()))?;
        write!(writer, "{}\t", escape(self.Password.clone()))?;
        write!(writer, "{}\t", escape(self.Arguments.clone()))?;
        write!(writer, "{}\t", escape(self.Component_.clone()))?;
        write!(writer, "{}\r\n", escape(self.Description.clone()))?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            ServiceInstall: unescape(parts[0].to_string()),
            Name: unescape(parts[1].to_string()),
            DisplayName: unescape(parts[2].to_string()),
            ServiceType: parts[3].parse().unwrap(),
            StartType: parts[4].parse().unwrap(),
            ErrorControl: parts[5].parse().unwrap(),
            LoadOrderGroup: unescape(parts[6].to_string()),
            Dependencies: unescape(parts[7].to_string()),
            StartName: unescape(parts[8].to_string()),
            Password: unescape(parts[9].to_string()),
            Arguments: unescape(parts[10].to_string()),
            Component_: unescape(parts[11].to_string()),
            Description: unescape(parts[12].to_string()),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct Shortcut {
    /// Primary key, non-localized token.
    pub Shortcut: String,
    /// Foreign key into the Directory table denoting the directory where the shortcut file is created.
    pub Directory_: String,
    /// The name of the shortcut to be created.
    pub Name: String,
    /// Foreign key into the Component table denoting the component whose selection gates the the shortcut creation/deletion.
    pub Component_: String,
    /// The shortcut target. This is usually a property that is expanded to a file or a folder that the shortcut points to.
    pub Target: String,
    /// The command-line arguments for the shortcut.
    pub Arguments: String,
    /// The description for the shortcut.
    pub Description: String,
    /// The hotkey for the shortcut. It has the virtual-key code for the key in the low-order byte, and the modifier flags in the high-order byte. 
    pub Hotkey: i16,
    /// Foreign key into the File table denoting the external icon file for the shortcut.
    pub Icon_: String,
    /// The icon index for the shortcut.
    pub IconIndex: i16,
    /// The show command for the application window.The following values may be used.
    pub ShowCmd: i16,
    /// Name of property defining location of working directory.
    pub WkDir: String,
}

impl Shortcut {
    #[rustfmt::skip]
    pub fn new(Shortcut: impl Into<String>, Directory_: impl Into<String>, Name: impl Into<String>, Component_: impl Into<String>, Target: impl Into<String>, Arguments: impl Into<String>, Description: impl Into<String>, Hotkey: impl Into<i16>, Icon_: impl Into<String>, IconIndex: impl Into<i16>, ShowCmd: impl Into<i16>, WkDir: impl Into<String>) -> Self {
        Self {
            Shortcut: Shortcut.into(),
            Directory_: Directory_.into(),
            Name: Name.into(),
            Component_: Component_.into(),
            Target: Target.into(),
            Arguments: Arguments.into(),
            Description: Description.into(),
            Hotkey: Hotkey.into(),
            Icon_: Icon_.into(),
            IconIndex: IconIndex.into(),
            ShowCmd: ShowCmd.into(),
            WkDir: WkDir.into(),
        }
    }
}

impl TableRow for Shortcut {
    const NAME: &'static str = "Shortcut";
    const COLUMNS: &'static str = "Shortcut\tDirectory_\tName\tComponent_\tTarget\tArguments\tDescription\tHotkey\tIcon_\tIconIndex\tShowCmd\tWkDir";
    const TYPES: &'static str = "s72\ts72\tl128\ts72\ts72\tS255\tL255\tI2\tS72\tI2\tI2\tS72";
    const META: &'static str = "Shortcut\tShortcut";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.Shortcut.clone()))?;
        write!(writer, "{}\t", escape(self.Directory_.clone()))?;
        write!(writer, "{}\t", escape(self.Name.clone()))?;
        write!(writer, "{}\t", escape(self.Component_.clone()))?;
        write!(writer, "{}\t", escape(self.Target.clone()))?;
        write!(writer, "{}\t", escape(self.Arguments.clone()))?;
        write!(writer, "{}\t", escape(self.Description.clone()))?;
        write!(writer, "{}\t", self.Hotkey)?;
        write!(writer, "{}\t", escape(self.Icon_.clone()))?;
        write!(writer, "{}\t", self.IconIndex)?;
        write!(writer, "{}\t", self.ShowCmd)?;
        write!(writer, "{}\r\n", escape(self.WkDir.clone()))?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            Shortcut: unescape(parts[0].to_string()),
            Directory_: unescape(parts[1].to_string()),
            Name: unescape(parts[2].to_string()),
            Component_: unescape(parts[3].to_string()),
            Target: unescape(parts[4].to_string()),
            Arguments: unescape(parts[5].to_string()),
            Description: unescape(parts[6].to_string()),
            Hotkey: parts[7].parse().unwrap(),
            Icon_: unescape(parts[8].to_string()),
            IconIndex: parts[9].parse().unwrap(),
            ShowCmd: parts[10].parse().unwrap(),
            WkDir: unescape(parts[11].to_string()),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct Signature {
    /// The table key. The Signature represents a unique file signature.
    pub Signature: String,
    /// The name of the file. This may contain a "short name|long name" pair.
    pub FileName: String,
    /// The minimum version of the file.
    pub MinVersion: String,
    /// The maximum version of the file.
    pub MaxVersion: String,
    /// The minimum size of the file.
    pub MinSize: i32,
    /// The maximum size of the file. 
    pub MaxSize: i32,
    /// The minimum creation date of the file.
    pub MinDate: i32,
    /// The maximum creation date of the file.
    pub MaxDate: i32,
    /// The languages supported by the file.
    pub Languages: String,
}

impl Signature {
    #[rustfmt::skip]
    pub fn new(Signature: impl Into<String>, FileName: impl Into<String>, MinVersion: impl Into<String>, MaxVersion: impl Into<String>, MinSize: impl Into<i32>, MaxSize: impl Into<i32>, MinDate: impl Into<i32>, MaxDate: impl Into<i32>, Languages: impl Into<String>) -> Self {
        Self {
            Signature: Signature.into(),
            FileName: FileName.into(),
            MinVersion: MinVersion.into(),
            MaxVersion: MaxVersion.into(),
            MinSize: MinSize.into(),
            MaxSize: MaxSize.into(),
            MinDate: MinDate.into(),
            MaxDate: MaxDate.into(),
            Languages: Languages.into(),
        }
    }
}

impl TableRow for Signature {
    const NAME: &'static str = "Signature";
    const COLUMNS: &'static str = "Signature\tFileName\tMinVersion\tMaxVersion\tMinSize\tMaxSize\tMinDate\tMaxDate\tLanguages";
    const TYPES: &'static str = "s72\ts255\tS20\tS20\tI4\tI4\tI4\tI4\tS255";
    const META: &'static str = "Signature\tSignature";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.Signature.clone()))?;
        write!(writer, "{}\t", escape(self.FileName.clone()))?;
        write!(writer, "{}\t", escape(self.MinVersion.clone()))?;
        write!(writer, "{}\t", escape(self.MaxVersion.clone()))?;
        write!(writer, "{}\t", self.MinSize)?;
        write!(writer, "{}\t", self.MaxSize)?;
        write!(writer, "{}\t", self.MinDate)?;
        write!(writer, "{}\t", self.MaxDate)?;
        write!(writer, "{}\r\n", escape(self.Languages.clone()))?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            Signature: unescape(parts[0].to_string()),
            FileName: unescape(parts[1].to_string()),
            MinVersion: unescape(parts[2].to_string()),
            MaxVersion: unescape(parts[3].to_string()),
            MinSize: parts[4].parse().unwrap(),
            MaxSize: parts[5].parse().unwrap(),
            MinDate: parts[6].parse().unwrap(),
            MaxDate: parts[7].parse().unwrap(),
            Languages: unescape(parts[8].to_string()),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct TextStyle {
    /// Name of the style. The primary key of this table. This name is embedded in the texts to indicate a style change.
    pub TextStyle: String,
    /// A string indicating the name of the font used. Required. The string must be at most 31 characters long.
    pub FaceName: String,
    /// The size of the font used. This size is given in our units (1/12 of the system font height). Assuming that the system font is set to 12 point size, this is equivalent to the point size.
    pub Size: i16,
    /// A long integer indicating the color of the string in the RGB format (Red, Green, Blue each 0-255, RGB = R + 256*G + 256^2*B).
    pub Color: i32,
    /// A combination of style bits.
    pub StyleBits: i16,
}

impl TextStyle {
    #[rustfmt::skip]
    pub fn new(TextStyle: impl Into<String>, FaceName: impl Into<String>, Size: impl Into<i16>, Color: impl Into<i32>, StyleBits: impl Into<i16>) -> Self {
        Self {
            TextStyle: TextStyle.into(),
            FaceName: FaceName.into(),
            Size: Size.into(),
            Color: Color.into(),
            StyleBits: StyleBits.into(),
        }
    }
}

impl TableRow for TextStyle {
    const NAME: &'static str = "TextStyle";
    const COLUMNS: &'static str = "TextStyle\tFaceName\tSize\tColor\tStyleBits";
    const TYPES: &'static str = "s72\ts32\ti2\tI4\tI2";
    const META: &'static str = "TextStyle\tTextStyle";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.TextStyle.clone()))?;
        write!(writer, "{}\t", escape(self.FaceName.clone()))?;
        write!(writer, "{}\t", self.Size)?;
        write!(writer, "{}\t", self.Color)?;
        write!(writer, "{}\r\n", self.StyleBits)?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            TextStyle: unescape(parts[0].to_string()),
            FaceName: unescape(parts[1].to_string()),
            Size: parts[2].parse().unwrap(),
            Color: parts[3].parse().unwrap(),
            StyleBits: parts[4].parse().unwrap(),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct TypeLib {
    /// The GUID that represents the library.
    pub LibID: String,
    /// The language of the library.
    pub Language: i16,
    /// Required foreign key into the Component Table, specifying the component for which to return a path when called through LocateComponent.
    pub Component_: String,
    /// The version of the library. The minor version is in the lower 8 bits of the integer. The major version is in the next 16 bits. 
    pub Version: i32,
    pub Description: String,
    /// Optional. The foreign key into the Directory table denoting the path to the help file for the type library.
    pub Directory_: String,
    /// Required foreign key into the Feature Table, specifying the feature to validate or install in order for the type library to be operational.
    pub Feature_: String,
    /// The cost associated with the registration of the typelib. This column is currently optional.
    pub Cost: i32,
}

impl TypeLib {
    #[rustfmt::skip]
    pub fn new(LibID: impl Into<String>, Language: impl Into<i16>, Component_: impl Into<String>, Version: impl Into<i32>, Description: impl Into<String>, Directory_: impl Into<String>, Feature_: impl Into<String>, Cost: impl Into<i32>) -> Self {
        Self {
            LibID: LibID.into(),
            Language: Language.into(),
            Component_: Component_.into(),
            Version: Version.into(),
            Description: Description.into(),
            Directory_: Directory_.into(),
            Feature_: Feature_.into(),
            Cost: Cost.into(),
        }
    }
}

impl TableRow for TypeLib {
    const NAME: &'static str = "TypeLib";
    const COLUMNS: &'static str = "LibID\tLanguage\tComponent_\tVersion\tDescription\tDirectory_\tFeature_\tCost";
    const TYPES: &'static str = "s38\ti2\ts72\tI4\tL128\tS72\ts38\tI4";
    const META: &'static str = "TypeLib\tLibID\tLanguage\tComponent_";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.LibID.clone()))?;
        write!(writer, "{}\t", self.Language)?;
        write!(writer, "{}\t", escape(self.Component_.clone()))?;
        write!(writer, "{}\t", self.Version)?;
        write!(writer, "{}\t", escape(self.Description.clone()))?;
        write!(writer, "{}\t", escape(self.Directory_.clone()))?;
        write!(writer, "{}\t", escape(self.Feature_.clone()))?;
        write!(writer, "{}\r\n", self.Cost)?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            LibID: unescape(parts[0].to_string()),
            Language: parts[1].parse().unwrap(),
            Component_: unescape(parts[2].to_string()),
            Version: parts[3].parse().unwrap(),
            Description: unescape(parts[4].to_string()),
            Directory_: unescape(parts[5].to_string()),
            Feature_: unescape(parts[6].to_string()),
            Cost: parts[7].parse().unwrap(),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct UIText {
    /// A unique key that identifies the particular string.
    pub Key: String,
    /// The localized version of the string.
    pub Text: String,
}

impl UIText {
    #[rustfmt::skip]
    pub fn new(Key: impl Into<String>, Text: impl Into<String>) -> Self {
        Self {
            Key: Key.into(),
            Text: Text.into(),
        }
    }
}

impl TableRow for UIText {
    const NAME: &'static str = "UIText";
    const COLUMNS: &'static str = "Key\tText";
    const TYPES: &'static str = "s72\tL255";
    const META: &'static str = "UIText\tKey";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.Key.clone()))?;
        write!(writer, "{}\r\n", escape(self.Text.clone()))?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            Key: unescape(parts[0].to_string()),
            Text: unescape(parts[1].to_string()),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct Upgrade {
    /// The UpgradeCode GUID belonging to the products in this set.
    pub UpgradeCode: String,
    /// The minimum ProductVersion of the products in this set.  The set may or may not include products with this particular version.
    pub VersionMin: String,
    /// The maximum ProductVersion of the products in this set.  The set may or may not include products with this particular version.
    pub VersionMax: String,
    /// A comma-separated list of languages for either products in this set or products not in this set.
    pub Language: String,
    /// The attributes of this product set.
    pub Attributes: i32,
    /// The list of features to remove when uninstalling a product from this set.  The default is "ALL".
    pub Remove: String,
    /// The property to set when a product in this set is found.
    pub ActionProperty: String,
}

impl Upgrade {
    #[rustfmt::skip]
    pub fn new(UpgradeCode: impl Into<String>, VersionMin: impl Into<String>, VersionMax: impl Into<String>, Language: impl Into<String>, Attributes: impl Into<i32>, Remove: impl Into<String>, ActionProperty: impl Into<String>) -> Self {
        Self {
            UpgradeCode: UpgradeCode.into(),
            VersionMin: VersionMin.into(),
            VersionMax: VersionMax.into(),
            Language: Language.into(),
            Attributes: Attributes.into(),
            Remove: Remove.into(),
            ActionProperty: ActionProperty.into(),
        }
    }
}

impl TableRow for Upgrade {
    const NAME: &'static str = "Upgrade";
    const COLUMNS: &'static str = "UpgradeCode\tVersionMin\tVersionMax\tLanguage\tAttributes\tRemove\tActionProperty";
    const TYPES: &'static str = "s38\tS20\tS20\tS255\ti4\tS255\ts72";
    const META: &'static str = "Upgrade\tUpgradeCode\tVersionMin\tVersionMax\tLanguage\tAttributes";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.UpgradeCode.clone()))?;
        write!(writer, "{}\t", escape(self.VersionMin.clone()))?;
        write!(writer, "{}\t", escape(self.VersionMax.clone()))?;
        write!(writer, "{}\t", escape(self.Language.clone()))?;
        write!(writer, "{}\t", self.Attributes)?;
        write!(writer, "{}\t", escape(self.Remove.clone()))?;
        write!(writer, "{}\r\n", escape(self.ActionProperty.clone()))?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            UpgradeCode: unescape(parts[0].to_string()),
            VersionMin: unescape(parts[1].to_string()),
            VersionMax: unescape(parts[2].to_string()),
            Language: unescape(parts[3].to_string()),
            Attributes: parts[4].parse().unwrap(),
            Remove: unescape(parts[5].to_string()),
            ActionProperty: unescape(parts[6].to_string()),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct Verb {
    /// The extension associated with the table row.
    pub Extension_: String,
    /// The verb for the command.
    pub Verb: String,
    /// Order within the verbs for a particular extension. Also used simply to specify the default verb.
    pub Sequence: i16,
    /// The command text.
    pub Command: String,
    /// Optional value for the command arguments.
    pub Argument: String,
}

impl Verb {
    #[rustfmt::skip]
    pub fn new(Extension_: impl Into<String>, Verb: impl Into<String>, Sequence: impl Into<i16>, Command: impl Into<String>, Argument: impl Into<String>) -> Self {
        Self {
            Extension_: Extension_.into(),
            Verb: Verb.into(),
            Sequence: Sequence.into(),
            Command: Command.into(),
            Argument: Argument.into(),
        }
    }
}

impl TableRow for Verb {
    const NAME: &'static str = "Verb";
    const COLUMNS: &'static str = "Extension_\tVerb\tSequence\tCommand\tArgument";
    const TYPES: &'static str = "s255\ts32\tI2\tL255\tL255";
    const META: &'static str = "Verb\tExtension_\tVerb";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.Extension_.clone()))?;
        write!(writer, "{}\t", escape(self.Verb.clone()))?;
        write!(writer, "{}\t", self.Sequence)?;
        write!(writer, "{}\t", escape(self.Command.clone()))?;
        write!(writer, "{}\r\n", escape(self.Argument.clone()))?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            Extension_: unescape(parts[0].to_string()),
            Verb: unescape(parts[1].to_string()),
            Sequence: parts[2].parse().unwrap(),
            Command: unescape(parts[3].to_string()),
            Argument: unescape(parts[4].to_string()),
        })
    }
}

#[derive(Debug, Default, Clone)]
pub struct _Validation {
    /// Name of table
    pub Table: String,
    /// Name of column
    pub Column: String,
    /// Whether the column is nullable
    pub Nullable: String,
    /// Minimum value allowed
    pub MinValue: i32,
    /// Maximum value allowed
    pub MaxValue: i32,
    /// For foreign key, Name of table to which data must link
    pub KeyTable: String,
    /// Column to which foreign key connects
    pub KeyColumn: i16,
    /// String category
    pub Category: String,
    /// Set of values that are permitted
    pub Set: String,
    /// Description of column
    pub Description: String,
}

impl _Validation {
    #[rustfmt::skip]
    pub fn new(Table: impl Into<String>, Column: impl Into<String>, Nullable: impl Into<String>, MinValue: impl Into<i32>, MaxValue: impl Into<i32>, KeyTable: impl Into<String>, KeyColumn: impl Into<i16>, Category: impl Into<String>, Set: impl Into<String>, Description: impl Into<String>) -> Self {
        Self {
            Table: Table.into(),
            Column: Column.into(),
            Nullable: Nullable.into(),
            MinValue: MinValue.into(),
            MaxValue: MaxValue.into(),
            KeyTable: KeyTable.into(),
            KeyColumn: KeyColumn.into(),
            Category: Category.into(),
            Set: Set.into(),
            Description: Description.into(),
        }
    }
}

impl TableRow for _Validation {
    const NAME: &'static str = "_Validation";
    const COLUMNS: &'static str = "Table\tColumn\tNullable\tMinValue\tMaxValue\tKeyTable\tKeyColumn\tCategory\tSet\tDescription";
    const TYPES: &'static str = "s32\ts32\ts4\tI4\tI4\tS255\tI2\tS32\tS255\tS255";
    const META: &'static str = "_Validation\tTable\tColumn";

    #[rustfmt::skip]
    fn write_row(&self, writer: &mut impl std::io::Write) -> std::io::Result<()> {
        write!(writer, "{}\t", escape(self.Table.clone()))?;
        write!(writer, "{}\t", escape(self.Column.clone()))?;
        write!(writer, "{}\t", escape(self.Nullable.clone()))?;
        write!(writer, "{}\t", self.MinValue)?;
        write!(writer, "{}\t", self.MaxValue)?;
        write!(writer, "{}\t", escape(self.KeyTable.clone()))?;
        write!(writer, "{}\t", self.KeyColumn)?;
        write!(writer, "{}\t", escape(self.Category.clone()))?;
        write!(writer, "{}\t", escape(self.Set.clone()))?;
        write!(writer, "{}\r\n", escape(self.Description.clone()))?;
        Ok(())
    }

    #[rustfmt::skip]
    fn read_row(reader: &mut impl std::io::BufRead) -> std::io::Result<Self> {
        let mut line = String::new();
        reader.read_line(&mut line)?;
        let parts: Vec<&str> = line.trim_end_matches(&['\r', '\n']).split('\t').collect();
        Ok(Self {
            Table: unescape(parts[0].to_string()),
            Column: unescape(parts[1].to_string()),
            Nullable: unescape(parts[2].to_string()),
            MinValue: parts[3].parse().unwrap(),
            MaxValue: parts[4].parse().unwrap(),
            KeyTable: unescape(parts[5].to_string()),
            KeyColumn: parts[6].parse().unwrap(),
            Category: unescape(parts[7].to_string()),
            Set: unescape(parts[8].to_string()),
            Description: unescape(parts[9].to_string()),
        })
    }
}

