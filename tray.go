package systray

func New(iconPath string, clientPath string) *Systray {
	return &Systray{_NewSystray(iconPath, clientPath)}
}

type Systray struct {
	*_Systray
}
