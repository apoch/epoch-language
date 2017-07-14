﻿/***************************************************************************

Copyright (c) Microsoft Corporation. All rights reserved.
This code is licensed under the Visual Studio SDK license terms.
THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.

***************************************************************************/

namespace EpochVSIX
{
    using System;
    using System.ComponentModel;
    using System.Runtime.InteropServices;
    using Microsoft.VisualStudio.Shell;
    using Microsoft.VisualStudio;
    using Microsoft.VisualStudio.Package;
    using Microsoft.VisualStudio.TextManager.Interop;
    using System.ComponentModel.Design;
    using System.ComponentModel.Composition;
    using Microsoft.VisualStudio.Editor;
    using Microsoft.VisualStudio.ComponentModelHost;

    /// <summary>
    /// This class implements the package exposed by this assembly.
    /// </summary>
    /// <remarks>
    /// This package is required if you want to define adds custom commands (ctmenu)
    /// or localized resources for the strings that appear in the New Project and Open Project dialogs.
    /// Creating project extensions or project types does not actually require a VSPackage.
    /// </remarks>
    [PackageRegistration(UseManagedResourcesOnly = true)]
    [Description("A custom project type based on CPS")]
    [Guid(VsPackage.PackageGuid)]
    [ProvideAutoLoad("{f1536ef8-92ec-443c-9ed7-fdadf150da82}")]         // UICONTEXT_SolutionExists
    [ProvideService(typeof(EpochLanguageService), ServiceName = "Epoch")]
    [ProvideLanguageExtension(VsPackage.LanguageServiceGuid, ".epoch")]
    [ProvideLanguageService(typeof(EpochLanguageService), "Epoch", 1, CodeSense = true, RequestStockColors = true, EnableCommenting = true)]
    public sealed class VsPackage : Package
    {
        /// <summary>
        /// The GUID for this package.
        /// </summary>
        public const string PackageGuid = "b95d8222-cdfc-44b4-9635-585db8a10b9f";

        /// <summary>
        /// The GUID for this project type.  It is unique with the project file extension and
        /// appears under the VS registry hive's Projects key.
        /// </summary>
        public const string ProjectTypeGuid = "7bd20c85-ab56-499d-b05b-d5f12345423c";

        public const string LanguageServiceGuid = "e0ddc0eb-23e7-45c7-8726-6fd20628992a";

        /// <summary>
        /// The file extension of this project type.  No preceding period.
        /// </summary>
        public const string ProjectExtension = "eprj";

        /// <summary>
        /// The default namespace this project compiles with, so that manifest
        /// resource names can be calculated for embedded resources.
        /// </summary>
        internal const string DefaultNamespace = "EpochVSIX";

        protected override void Initialize()
        {
            base.Initialize();

            IServiceContainer container = this as IServiceContainer;
            var service = new EpochLanguageService(((IComponentModel)GetGlobalService(typeof(SComponentModel))).GetService<IVsEditorAdaptersFactoryService>());
            service.SetSite(this);
            container.AddService(typeof(EpochLanguageService), service, true);
        }
    }
}
