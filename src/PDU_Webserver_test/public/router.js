document.addEventListener('DOMContentLoaded', function () {

    const pages = document.querySelectorAll('.page');

    function showPage(id) {
        pages.forEach(p => p.classList.remove('active'));

        const page = document.getElementById(id);
        if (page) page.classList.add('active');

        // IEC dashboard esetén render
        if (id === 'iecDashboardPage') {
            renderDashboard();
        }
    }

    document.getElementById('welcome').onclick = (e) => {
        e.preventDefault();
        showPage('welcomePage');
        history.pushState({page:'welcomePage'}, '', '#welcome');
    };

    document.getElementById('iecDashboard').onclick = (e) => {
        e.preventDefault();
        showPage('iecDashboardPage');
        showDashboard();
        history.pushState({page:'iecDashboardPage'}, '', '#iec');
    };

    document.getElementById('pduSettings').onclick = (e) => {
        e.preventDefault();
        showPage('pduSettingsPage');
        history.pushState({page:'pduSettingsPage'}, '', '#pdu');
    };

    document.getElementById('iecSettings').onclick = (e) => {
        e.preventDefault();
        showPage('iecSettingsPage');
        history.pushState({page:'iecSettingsPage'}, '', '#iecsettings');
    };

    document.getElementById('about').onclick = (e) => {
        e.preventDefault();
        showPage('aboutPage');
        history.pushState({page:'aboutPage'}, '', '#about');
    };

    window.addEventListener('popstate', function (event) {
        if (event.state && event.state.page) {
            showPage(event.state.page);
        }
    });

    // default
    showPage('welcomePage');
});