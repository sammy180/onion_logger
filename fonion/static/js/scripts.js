let currentFieldIndex = 0;
let scrollLabels = [];
let boxIds = [];

async function fetchScrollLabels() {
    try {
        const response = await fetch('/setup');
        const data = await response.json();
        scrollLabels = data.scroll_labels;
        boxIds = data.box_ids;
        console.log('Scroll labels and box IDs fetched:', scrollLabels, boxIds);
    } catch (error) {
        console.error('Error fetching scroll labels:', error);
    }
}

async function updateQuadrants() {
    if (scrollLabels.length === 0 || boxIds.length === 0) return;

    try {
        const field = scrollLabels[currentFieldIndex];
        const response = await fetch(`/get_data?field=${field}`);
        const data = await response.json();

        // Update each quadrant and check time difference
        for (let i = 1; i <= 4; i++) {
            const quadrant = document.querySelector(`.quadrant:nth-child(${i})`);
            const boxData = data[`box${i}`];
            
            quadrant.querySelector('.title').textContent = `Box ${boxIds[i-1]}`;
            quadrant.querySelector('.parameter').textContent = `${field}: ${boxData.value}`;
            quadrant.querySelector('.time_diff').textContent = `last meas: ${boxData.time_diff} min ago`;
            
            // Add or remove alert class based on time difference
            if (boxData.time_diff > 5) {
                quadrant.classList.add('alert');
            } else {
                quadrant.classList.remove('alert');
            }
        }

    } catch (error) {
        console.error('Error fetching data:', error);
    }
}

function nextField() {
    currentFieldIndex = (currentFieldIndex + 1) % scrollLabels.length;
    updateQuadrants();
    console.log('nextField has been run');
}

function prevField() {
    currentFieldIndex = (currentFieldIndex - 1 + scrollLabels.length) % scrollLabels.length;
    updateQuadrants();
    console.log('prevField has been run');
}

// Fetch scroll labels and update quadrants when the page loads
window.onload = async () => {
    await fetchScrollLabels();
    updateQuadrants();
};